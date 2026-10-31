import Link from "next/link";
import { notFound } from "next/navigation";
import { db } from "@/lib/db";
import { assertOwnerOrStaff, requireUser } from "@/lib/auth/rbac";
import { availableTransitions } from "@/lib/subscription-state";
import { formatMoney } from "@/lib/money";
import { SUB_STATUSES } from "@/lib/enums";
import { Topbar } from "@/components/layout/Topbar";
import { StatusBadge } from "@/components/ui/Badge";
import { LiveRefresh } from "@/components/ui/LiveRefresh";
import { TransitionButtons } from "@/components/ui/TransitionButtons";

const date = (d: Date | null) => (d ? d.toLocaleDateString("en-IN", { day: "numeric", month: "short", year: "numeric" }) : "No end date");

export default async function SubscriptionPage({ params }: { params: { id: string } }) {
  const user = await requireUser();
  const sub = await db.subscription.findUnique({
    where: { id: params.id },
    include: {
      customer: { select: { id: true, name: true, email: true } },
      plan: true,
      lines: { include: { product: true, variant: true, taxRule: true }, orderBy: { id: "asc" } },
      invoices: { orderBy: { periodStart: "desc" }, take: 12, select: { id: true, number: true, status: true, total: true, dueDate: true, periodStart: true } },
    },
  });
  if (!sub) notFound();
  assertOwnerOrStaff(user, sub.customer.id); // customers can only open their own subscriptions

  const lines = sub.lines.map((l) => {
    const net = l.quantity * l.unitPrice;
    const rate = l.taxRule?.rateBps ?? 0;
    const tax = Math.round((net * rate) / 10_000);
    return { ...l, net, rate, tax, total: net + tax };
  });
  const subtotal = lines.reduce((s, l) => s + l.net, 0);
  const tax = lines.reduce((s, l) => s + l.tax, 0);

  const isStaff = user.role !== "CUSTOMER";
  const next = availableTransitions(sub, user.role);
  const stepIndex = SUB_STATUSES.indexOf(sub.status as (typeof SUB_STATUSES)[number]);
  const displayStatus = sub.paused ? "PAUSED" : sub.status;

  return (
    <>
      <Topbar crumbs={[{ label: "Subscriptions", href: "/subscriptions" }, { label: sub.number }]} />
      <LiveRefresh seconds={15} />
      <main className="mx-auto w-full max-w-5xl space-y-6 p-4 sm:p-6">
        <header className="flex flex-wrap items-start justify-between gap-4">
          <div>
            <div className="flex items-center gap-3">
              <h1 className="text-2xl font-semibold text-[#1E293B]">{sub.number}</h1>
              <StatusBadge status={displayStatus} />
            </div>
            <p className="mt-1 text-sm text-slate-600">{sub.customer.name} · {sub.customer.email}</p>
          </div>
          {isStaff && (
            <TransitionButtons
              id={sub.id}
              next={next}
              canPause={sub.status === "ACTIVE" && sub.plan.pausable}
              paused={sub.paused}
              canRenew={sub.plan.renewable && (sub.status === "ACTIVE" || sub.status === "CLOSED")}
            />
          )}
        </header>

        <ol aria-label="Lifecycle" className="grid grid-cols-5 gap-1 text-center text-xs sm:text-sm">
          {SUB_STATUSES.map((s, i) => (
            <li key={s} aria-current={i === stepIndex ? "step" : undefined} className={`rounded-lg px-1 py-2 ${i < stepIndex ? "bg-indigo-100 text-indigo-900" : i === stepIndex ? "bg-[#6366F1] font-medium text-white" : "bg-slate-100 text-slate-500"}`}>
              {s.charAt(0) + s.slice(1).toLowerCase()}
            </li>
          ))}
        </ol>

        <section className="grid gap-4 rounded-xl border border-slate-200 bg-white p-5 sm:grid-cols-2 lg:grid-cols-4">
          {[
            ["Plan", sub.plan.name],
            ["Billing", sub.plan.frequency.charAt(0) + sub.plan.frequency.slice(1).toLowerCase()],
            ["Start", date(sub.startDate)],
            ["Expiry", date(sub.expiryDate)],
            ["Next invoice", sub.status === "ACTIVE" ? (sub.paused ? "Paused" : date(sub.nextBillingDate)) : "—"],
            ["Payment terms", sub.paymentTerms.replace("NET_", "Net ") + " days"],
            ["Quotation valid until", sub.status === "QUOTATION" ? date(sub.quotationValidUntil) : "—"],
          ].map(([k, v]) => (
            <div key={k}>
              <dt className="text-xs text-slate-500">{k}</dt>
              <dd className="mt-0.5 text-sm font-medium text-slate-900">{v}</dd>
            </div>
          ))}
        </section>

        <section className="rounded-xl border border-slate-200 bg-white">
          <h2 className="p-5 pb-3 font-semibold text-[#1E293B]">Order lines</h2>
          <div className="overflow-x-auto">
            <table className="w-full min-w-[640px] text-left text-sm">
              <thead className="border-y border-slate-200 bg-slate-50 text-slate-600">
                <tr>
                  <th className="px-5 py-2 font-medium">Product</th>
                  <th className="px-3 py-2 text-right font-medium">Qty</th>
                  <th className="px-3 py-2 text-right font-medium">Unit price</th>
                  <th className="px-3 py-2 text-right font-medium">Tax</th>
                  <th className="px-5 py-2 text-right font-medium">Amount</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {lines.map((l) => (
                  <tr key={l.id}>
                    <td className="px-5 py-3">{l.product.name}{l.variant && <span className="block text-xs text-slate-500">{l.variant.attribute}: {l.variant.value}</span>}</td>
                    <td className="px-3 py-3 text-right tabular-nums">{l.quantity}</td>
                    <td className="px-3 py-3 text-right tabular-nums">{formatMoney(l.unitPrice)}</td>
                    <td className="px-3 py-3 text-right tabular-nums">{l.taxRule ? `${l.taxRule.name} · ${formatMoney(l.tax)}` : "—"}</td>
                    <td className="px-5 py-3 text-right font-medium tabular-nums">{formatMoney(l.total)}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
          <dl className="ml-auto w-full max-w-xs space-y-1 p-5 text-sm">
            <div className="flex justify-between"><dt className="text-slate-600">Subtotal</dt><dd className="tabular-nums">{formatMoney(subtotal)}</dd></div>
            <div className="flex justify-between"><dt className="text-slate-600">Tax</dt><dd className="tabular-nums">{formatMoney(tax)}</dd></div>
            <div className="flex justify-between border-t border-slate-200 pt-2 text-base font-semibold text-[#1E293B]"><dt>Total per {sub.plan.frequency.toLowerCase().replace("ly", "").replace("dai", "day")}</dt><dd className="tabular-nums">{formatMoney(subtotal + tax)}</dd></div>
            <p className="pt-1 text-xs text-slate-500">Active discounts are applied automatically on the next invoice.</p>
          </dl>
        </section>

        <section className="rounded-xl border border-slate-200 bg-white">
          <h2 className="p-5 pb-3 font-semibold text-[#1E293B]">Invoices</h2>
          {sub.invoices.length === 0 ? (
            <p className="px-5 pb-6 text-sm text-slate-500">No invoices yet. The first one is created on billing day.</p>
          ) : (
            <ul className="divide-y divide-slate-100">
              {sub.invoices.map((inv) => (
                <li key={inv.id} className="flex flex-wrap items-center justify-between gap-2 px-5 py-3 text-sm">
                  <Link href={`/invoices/${inv.id}`} className="font-medium text-[#6366F1] hover:underline">{inv.number}</Link>
                  <span className="text-slate-500">Due {date(inv.dueDate)}</span>
                  <StatusBadge status={inv.status !== "PAID" && inv.status === "CONFIRMED" && inv.dueDate < new Date() ? "OVERDUE" : inv.status} />
                  <span className="tabular-nums">{formatMoney(inv.total)}</span>
                </li>
              ))}
            </ul>
          )}
        </section>
      </main>
    </>
  );
}
