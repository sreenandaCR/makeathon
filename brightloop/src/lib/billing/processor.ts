import { Prisma } from "@prisma/client";
import { db } from "../db";
import { addDays, addPeriod } from "../dates";
import { PAYMENT_TERMS, type Frequency, type PaymentTerms } from "../enums";
import { computeInvoice, eligibleRules, type PricedLine } from "./pricing";

export interface BillingReport {
  scanned: number;
  invoicesCreated: number;
  skippedAlreadyBilled: number;
  closed: number;
  failures: Array<{ subscriptionId: string; error: string }>;
}

const BATCH = 200;
const MAX_CATCH_UP_PERIODS = 36; // never loop forever on bad data, but still bill every missed cycle

/**
 * Billing-day entry point. Safe to run repeatedly and concurrently:
 *  1. each period is "claimed" with a compare-and-set on nextBillingDate, and
 *  2. @@unique([subscriptionId, periodStart]) on Invoice is the hard backstop.
 * Missed cycles (job was down) are billed one period at a time, so nothing is skipped.
 */
export async function runBillingCycle(now = new Date()): Promise<BillingReport> {
  const report: BillingReport = { scanned: 0, invoicesCreated: 0, skippedAlreadyBilled: 0, closed: 0, failures: [] };
  let cursor: string | undefined;

  for (;;) {
    const due = await db.subscription.findMany({
      where: { status: "ACTIVE", paused: false, nextBillingDate: { lte: now } },
      select: { id: true },
      orderBy: { id: "asc" },
      take: BATCH,
      ...(cursor ? { cursor: { id: cursor }, skip: 1 } : {}),
    });
    if (due.length === 0) break;
    cursor = due[due.length - 1].id;

    for (const { id } of due) {
      report.scanned++;
      try {
        const r = await billSubscription(id, now);
        report.invoicesCreated += r.created;
        report.skippedAlreadyBilled += r.skipped;
        report.closed += r.closed ? 1 : 0;
      } catch (e) {
        // One bad subscription must not stop the other hundreds.
        report.failures.push({ subscriptionId: id, error: e instanceof Error ? e.message : String(e) });
      }
    }
  }

  report.closed += await closeExpiredSubscriptions(now);
  return report;
}

async function billSubscription(id: string, now: Date) {
  const out = { created: 0, skipped: 0, closed: false };

  for (let i = 0; i < MAX_CATCH_UP_PERIODS; i++) {
    const outcome = await billOnePeriod(id, now);
    if (outcome === "none") break;
    if (outcome === "created") out.created++;
    if (outcome === "skipped") out.skipped++;
    if (outcome === "closed") {
      out.closed = true;
      break;
    }
  }
  return out;
}

type PeriodOutcome = "created" | "skipped" | "closed" | "none";

async function billOnePeriod(id: string, now: Date): Promise<PeriodOutcome> {
  return db.$transaction(async (tx) => {
    const sub = await tx.subscription.findUnique({
      where: { id },
      include: { plan: true, lines: { include: { product: true, variant: true, taxRule: true } } },
    });
    if (!sub || sub.status !== "ACTIVE" || sub.paused || !sub.nextBillingDate || sub.nextBillingDate > now) return "none";

    const frequency = sub.plan.frequency as Frequency;
    const periodStart = sub.nextBillingDate;
    const periodEnd = addPeriod(periodStart, frequency);

    // Past the contract end: nothing more to bill.
    if (sub.expiryDate && periodStart >= sub.expiryDate) {
      await tx.subscription.update({
        where: { id },
        data: sub.plan.autoClose ? { status: "CLOSED", closedAt: now, nextBillingDate: null } : { nextBillingDate: null },
      });
      return sub.plan.autoClose ? "closed" : "none";
    }

    // Claim the period. If another worker already advanced it, count = 0 and we back off.
    const claim = await tx.subscription.updateMany({
      where: { id, nextBillingDate: periodStart, status: "ACTIVE" },
      data: { nextBillingDate: periodEnd },
    });
    if (claim.count !== 1) return "skipped";

    const priced: PricedLine[] = sub.lines.map((l) => ({
      productId: l.productId,
      description: l.variant ? `${l.product.name} (${l.variant.attribute}: ${l.variant.value})` : l.product.name,
      quantity: l.quantity,
      unitPrice: l.unitPrice,
      taxRateBps: l.taxRule?.isActive ? l.taxRule.rateBps : 0,
    }));

    // Discounts: pick eligible rules, then reserve a redemption atomically (guards maxRedemptions under concurrency).
    const candidates = await tx.discountRule.findMany({
      where: { isActive: true, startsAt: { lte: now }, endsAt: { gte: now } },
      orderBy: { createdAt: "asc" },
    });
    const reserved = [];
    for (const rule of eligibleRules(candidates, priced, now)) {
      if (rule.maxRedemptions == null) {
        reserved.push(rule);
        continue;
      }
      const res = await tx.discountRule.updateMany({
        where: { id: rule.id, redemptionCount: { lt: rule.maxRedemptions } },
        data: { redemptionCount: { increment: 1 } },
      });
      if (res.count === 1) reserved.push(rule);
    }

    const calc = computeInvoice(priced, reserved);
    const termsDays = PAYMENT_TERMS[sub.paymentTerms as PaymentTerms] ?? 15;
    const number = `INV-${sub.number.replace(/^SUB-/, "")}-${periodStart.toISOString().slice(0, 10).replace(/-/g, "")}`;

    const invoice = await tx.invoice.create({
      data: {
        number,
        subscriptionId: sub.id,
        customerId: sub.customerId,
        status: process.env.BILLING_AUTO_CONFIRM === "true" ? "CONFIRMED" : "DRAFT",
        periodStart,
        periodEnd,
        issueDate: now,
        dueDate: addDays(now, termsDays),
        subtotal: calc.subtotal,
        discountTotal: calc.discountTotal,
        taxTotal: calc.taxTotal,
        total: calc.total,
        lines: {
          create: calc.lines.map((l) => ({
            description: l.description,
            quantity: l.quantity,
            unitPrice: l.unitPrice,
            discount: l.discount,
            taxRateBps: l.taxRateBps,
            taxAmount: l.taxAmount,
            total: l.total,
          })),
        },
      },
    });
    if (calc.applied.length) {
      await tx.discountRedemption.createMany({
        data: calc.applied.map((a) => ({ discountId: a.ruleId, invoiceId: invoice.id, amount: a.amount })),
      });
    }
    return "created";
  }).catch((e) => {
    // Unique-violation = another worker billed this period first. Roll back (done) and move on.
    if (e instanceof Prisma.PrismaClientKnownRequestError && e.code === "P2002") return "skipped" as const;
    throw e;
  });
}

/** Sweep: auto-close plans whose contract ended and that have nothing left to bill. */
async function closeExpiredSubscriptions(now: Date): Promise<number> {
  const res = await db.subscription.updateMany({
    where: { status: "ACTIVE", expiryDate: { lte: now }, nextBillingDate: null, plan: { autoClose: true } },
    data: { status: "CLOSED", closedAt: now },
  });
  return res.count;
}
