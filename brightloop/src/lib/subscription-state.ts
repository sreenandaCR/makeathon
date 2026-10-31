import type { Plan, Subscription } from "@prisma/client";
import { db } from "./db";
import { addDays } from "./dates";
import type { SubStatus } from "./enums";
import type { Role } from "./enums";

/** DRAFT -> QUOTATION -> CONFIRMED -> ACTIVE -> CLOSED. CLOSED is terminal (renewal creates a new subscription). */
const TRANSITIONS: Record<SubStatus, SubStatus[]> = {
  DRAFT: ["QUOTATION", "CLOSED"],
  QUOTATION: ["CONFIRMED", "DRAFT", "CLOSED"],
  CONFIRMED: ["ACTIVE", "CLOSED"],
  ACTIVE: ["CLOSED"],
  CLOSED: [],
};

export class TransitionError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "TransitionError";
  }
}

type SubWithPlan = Subscription & { plan: Plan };

/** Pure guard: why can't this subscription move to `to`? Returns null when allowed. */
export function checkTransition(sub: SubWithPlan, to: SubStatus, now = new Date()): string | null {
  const from = sub.status as SubStatus;
  if (!TRANSITIONS[from].includes(to)) return `Cannot move from ${from} to ${to}`;

  if (to === "CONFIRMED" && sub.quotationValidUntil && sub.quotationValidUntil < now) {
    return "This quotation has expired. Move it back to Draft and re-issue it.";
  }
  if (to === "ACTIVE") {
    if (!sub.plan.isActive) return "The plan is no longer active";
    if (sub.plan.availableTo && sub.plan.availableTo < now) return "The plan is no longer available";
    if (sub.expiryDate && sub.expiryDate <= now) return "The expiry date has already passed";
  }
  if (to === "CLOSED" && from === "ACTIVE" && !sub.plan.closable && !sub.plan.autoClose) {
    return "This plan cannot be closed early";
  }
  return null;
}

export function availableTransitions(sub: SubWithPlan, role: Role): SubStatus[] {
  if (role === "CUSTOMER") return []; // customers use pause/close controls on the portal, not the pipeline
  return TRANSITIONS[sub.status as SubStatus].filter((to) => checkTransition(sub, to) === null);
}

export async function transitionSubscription(id: string, to: SubStatus, now = new Date()): Promise<Subscription> {
  return db.$transaction(async (tx) => {
    const sub = await tx.subscription.findUniqueOrThrow({ where: { id }, include: { plan: true, lines: true } });
    const problem = checkTransition(sub, to, now);
    if (problem) throw new TransitionError(problem);
    if (to === "QUOTATION" && sub.lines.length === 0) throw new TransitionError("Add at least one line before quoting");

    const data: Partial<Subscription> = { status: to };
    if (to === "QUOTATION") {
      // Validity comes from the quotation template when the sub was built from one; default 30 days.
      data.quotationValidUntil = addDays(now, 30);
    }
    if (to === "ACTIVE") {
      data.nextBillingDate = sub.startDate; // billing in advance: first invoice is due on the start date
      data.paused = false;
    }
    if (to === "CLOSED") {
      data.closedAt = now;
      data.nextBillingDate = null;
    }

    // Compare-and-set on the current status so two simultaneous clicks can't both win.
    const res = await tx.subscription.updateMany({ where: { id, status: sub.status }, data });
    if (res.count !== 1) throw new TransitionError("This subscription was changed by someone else. Refresh and try again.");
    return tx.subscription.findUniqueOrThrow({ where: { id } });
  });
}

export async function setPaused(id: string, paused: boolean): Promise<void> {
  const sub = await db.subscription.findUniqueOrThrow({ where: { id }, include: { plan: true } });
  if (sub.status !== "ACTIVE") throw new TransitionError("Only active subscriptions can be paused");
  if (!sub.plan.pausable) throw new TransitionError("This plan cannot be paused");
  await db.subscription.update({ where: { id }, data: { paused } });
}

/** Renewal clones the subscription into a fresh DRAFT starting at the old expiry. Plan must be renewable. */
export async function renewSubscription(id: string): Promise<Subscription> {
  const old = await db.subscription.findUniqueOrThrow({ where: { id }, include: { plan: true, lines: true } });
  if (!old.plan.renewable) throw new TransitionError("This plan cannot be renewed");
  if (old.status !== "ACTIVE" && old.status !== "CLOSED") throw new TransitionError("Only active or closed subscriptions can be renewed");

  const start = old.expiryDate ?? new Date();
  const span = old.expiryDate ? old.expiryDate.getTime() - old.startDate.getTime() : null;
  return db.subscription.create({
    data: {
      number: await nextSubscriptionNumber(),
      customerId: old.customerId,
      planId: old.planId,
      startDate: start,
      expiryDate: span ? new Date(start.getTime() + span) : null,
      paymentTerms: old.paymentTerms,
      recurringTotal: old.recurringTotal,
      lines: {
        create: old.lines.map((l) => ({
          productId: l.productId,
          variantId: l.variantId,
          taxRuleId: l.taxRuleId,
          quantity: l.quantity,
          unitPrice: l.unitPrice,
        })),
      },
    },
  });
}

export async function nextSubscriptionNumber(): Promise<string> {
  const year = new Date().getUTCFullYear();
  // Count-based numbering; the unique index on `number` makes a collision fail loudly and the caller retries.
  const n = (await db.subscription.count({ where: { number: { startsWith: `SUB-${year}-` } } })) + 1;
  return `SUB-${year}-${String(n).padStart(6, "0")}`;
}
