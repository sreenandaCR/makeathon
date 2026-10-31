import { Prisma } from "@prisma/client";
import { db } from "./db";
import { addDays } from "./dates";
import type { DashboardFilters } from "./validation";

export interface FinanceSummary {
  activeSubscriptions: number;
  mrr: number;
  arr: number;
  revenueCollected: number;
  outstanding: number;
  overdueTotal: number;
  overdueCount: number;
  aging: Array<{ bucket: "0-30" | "31-60" | "61-90" | "90+"; amount: number; count: number }>;
}

function sinceFor(range: DashboardFilters["range"], now: Date): Date | undefined {
  if (range === "30d") return addDays(now, -30);
  if (range === "90d") return addDays(now, -90);
  if (range === "ytd") return new Date(Date.UTC(now.getUTCFullYear(), 0, 1));
  return undefined;
}

const BUCKETS = [
  { bucket: "0-30", from: 0, to: 30 },
  { bucket: "31-60", from: 31, to: 60 },
  { bucket: "61-90", from: 61, to: 90 },
  { bucket: "90+", from: 91, to: null },
] as const;

/**
 * Every figure is a single indexed aggregate (no row-by-row loops), run in parallel.
 * With the indexes in schema.prisma this stays well under the 2s budget at tens of thousands of subscriptions.
 */
export async function getFinanceSummary(f: DashboardFilters, now = new Date()): Promise<FinanceSummary> {
  const since = sinceFor(f.range, now);
  const planFilter = f.frequency ? { plan: { frequency: f.frequency } } : {};
  const invoiceScope: Prisma.InvoiceWhereInput = { status: "CONFIRMED", ...(f.frequency ? { subscription: planFilter } : {}) };

  const freqSql = f.frequency ? Prisma.sql`AND p."frequency" = ${f.frequency}` : Prisma.empty;

  const [active, mrrRows, revenue, outstanding, overdue, ...bucketRows] = await Promise.all([
    db.subscription.count({ where: { status: "ACTIVE", paused: false, ...planFilter } }),
    // Normalise every plan to a monthly figure in SQL (integer math keeps it portable across SQLite/Postgres).
    db.$queryRaw<Array<{ mrr: bigint | number | null }>>(Prisma.sql`
      SELECT SUM(CASE p."frequency"
        WHEN 'DAILY'   THEN s."recurringTotal" * 30
        WHEN 'WEEKLY'  THEN s."recurringTotal" * 52 / 12
        WHEN 'MONTHLY' THEN s."recurringTotal"
        WHEN 'YEARLY'  THEN s."recurringTotal" / 12
      END) AS mrr
      FROM "Subscription" s JOIN "Plan" p ON p."id" = s."planId"
      WHERE s."status" = 'ACTIVE' AND s."paused" = ${false} ${freqSql}`),
    db.payment.aggregate({
      _sum: { amount: true },
      where: { ...(since ? { paidAt: { gte: since } } : {}), ...(f.frequency ? { invoice: { subscription: planFilter } } : {}) },
    }),
    db.invoice.aggregate({ _sum: { total: true, amountPaid: true }, where: invoiceScope }),
    db.invoice.aggregate({ _sum: { total: true, amountPaid: true }, _count: true, where: { ...invoiceScope, dueDate: { lt: now } } }),
    ...BUCKETS.map((b) =>
      db.invoice.aggregate({
        _sum: { total: true, amountPaid: true },
        _count: true,
        where: {
          ...invoiceScope,
          dueDate: { lt: addDays(now, -b.from), ...(b.to != null ? { gte: addDays(now, -(b.to + 1)) } : {}) },
        },
      }),
    ),
  ]);

  const mrr = Number(mrrRows[0]?.mrr ?? 0);
  const owed = (a: { _sum: { total: number | null; amountPaid: number | null } }) => (a._sum.total ?? 0) - (a._sum.amountPaid ?? 0);

  return {
    activeSubscriptions: active,
    mrr,
    arr: mrr * 12,
    revenueCollected: revenue._sum.amount ?? 0,
    outstanding: owed(outstanding),
    overdueTotal: owed(overdue),
    overdueCount: overdue._count,
    aging: BUCKETS.map((b, i) => ({ bucket: b.bucket, amount: owed(bucketRows[i]), count: bucketRows[i]._count })),
  };
}

export async function getOverdueInvoices(f: DashboardFilters, page = 1, pageSize = 15, now = new Date()) {
  const b = BUCKETS.find((x) => x.bucket === f.aging);
  const where: Prisma.InvoiceWhereInput = {
    status: "CONFIRMED",
    dueDate: b
      ? { lt: addDays(now, -b.from), ...(b.to != null ? { gte: addDays(now, -(b.to + 1)) } : {}) }
      : { lt: now },
    ...(f.frequency ? { subscription: { plan: { frequency: f.frequency } } } : {}),
  };
  const [rows, total] = await Promise.all([
    db.invoice.findMany({
      where,
      orderBy: { dueDate: "asc" },
      skip: (page - 1) * pageSize,
      take: pageSize,
      select: {
        id: true, number: true, dueDate: true, total: true, amountPaid: true,
        customer: { select: { name: true, email: true } },
        subscription: { select: { id: true, number: true } },
      },
    }),
    db.invoice.count({ where }),
  ]);
  return { rows, total, pageCount: Math.max(1, Math.ceil(total / pageSize)) };
}
