"use server";

import { revalidatePath } from "next/cache";
import { db } from "@/lib/db";
import { assertRole, ForbiddenError } from "@/lib/auth/rbac";
import { paymentSchema } from "@/lib/validation";
import type { ActionResult } from "./auth";

/** Invoice state machine: DRAFT -> CONFIRMED -> PAID. DRAFT/CONFIRMED -> CANCELLED (only if no payments). */
export async function confirmInvoiceAction(invoiceId: string): Promise<ActionResult> {
  return guarded(async () => {
    await assertRole("ADMIN", "STAFF");
    const r = await db.invoice.updateMany({ where: { id: invoiceId, status: "DRAFT" }, data: { status: "CONFIRMED" } });
    if (r.count !== 1) return { ok: false, error: "Only draft invoices can be confirmed" };
    revalidatePath("/invoices");
    return { ok: true };
  });
}

export async function cancelInvoiceAction(invoiceId: string): Promise<ActionResult> {
  return guarded(async () => {
    await assertRole("ADMIN", "STAFF");
    const r = await db.invoice.updateMany({
      where: { id: invoiceId, status: { in: ["DRAFT", "CONFIRMED"] }, amountPaid: 0 },
      data: { status: "CANCELLED" },
    });
    if (r.count !== 1) return { ok: false, error: "This invoice can't be cancelled (already paid or partly paid)" };
    revalidatePath("/invoices");
    return { ok: true };
  });
}

export async function recordPaymentAction(input: unknown): Promise<ActionResult> {
  return guarded(async () => {
    const user = await assertRole("ADMIN", "STAFF");
    const parsed = paymentSchema.safeParse(input);
    if (!parsed.success) return { ok: false, error: "Check the highlighted fields", fieldErrors: parsed.error.flatten().fieldErrors };
    const p = parsed.data;

    return db.$transaction(async (tx) => {
      const inv = await tx.invoice.findUnique({ where: { id: p.invoiceId } });
      if (!inv) return { ok: false as const, error: "Invoice not found" };
      if (inv.status !== "CONFIRMED") return { ok: false as const, error: "Confirm the invoice before recording payments" };
      const outstanding = inv.total - inv.amountPaid;
      if (p.amount > outstanding) return { ok: false as const, error: "Amount is more than the outstanding balance", fieldErrors: { amount: ["Exceeds outstanding balance"] } };

      await tx.payment.create({ data: { ...p, recordedById: user.id } });
      // Guarded increment: refuses if a concurrent payment already consumed the balance.
      const upd = await tx.invoice.updateMany({
        where: { id: inv.id, status: "CONFIRMED", amountPaid: inv.amountPaid },
        data: { amountPaid: inv.amountPaid + p.amount, status: p.amount === outstanding ? "PAID" : "CONFIRMED" },
      });
      if (upd.count !== 1) throw new Error("Invoice changed while saving. Try again.");
      revalidatePath("/invoices");
      revalidatePath("/dashboard");
      return { ok: true as const };
    });
  });
}

async function guarded(fn: () => Promise<ActionResult>): Promise<ActionResult> {
  try {
    return await fn();
  } catch (e) {
    if (e instanceof ForbiddenError) return { ok: false, error: e.message };
    return { ok: false, error: e instanceof Error ? e.message : "Something went wrong" };
  }
}
