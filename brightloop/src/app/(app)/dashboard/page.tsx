import Link from "next/link";
import { requireUser } from "@/lib/auth/rbac";
import { getFinanceSummary, getOverdueInvoices } from "@/lib/analytics";
import { dashboardFilterSchema } from "@/lib/validation";
import { formatMoney } from "@/lib/money";
import { Topbar } from "@/components/layout/Topbar";
import { FilterBar } from "@/components/dashboard/FilterBar";
import { LiveRefresh } from "@/components/ui/LiveRefresh";

type SP = Record<string, string | string[] | undefined>;
const first = (v: string | string[] | undefined) => (Array.isArray(v) ? v[0] : v);

export default async function DashboardPage({ searchParams }: { searchParams: SP }) {
  await requireUser("ADMIN", "STAFF");
  const filters = dashboardFilterSchema.catch({ range: "30d", aging: "all" }).parse({
    range: first(searchParams.range),
    frequency: first(searchParams.frequency),
    aging: first(searchParams.aging),
  });
  const page = Math.max(1, Number(first(searchParams.page)) || 1);

  const [summary, overdue] = await Promise.all([getFinanceSummary(filters), getOverdueInvoices(filters, page)]);
  const maxBucket = Math.max(1, ...summary.aging.map((a) => a.amount));

  const kpis = [
    { label: "Active subscriptions", value: summary.activeSubscriptions.toLocaleString("en-IN") },
    { label: "Monthly recurring revenue", value: formatMoney(summary.mrr), hint: `ARR ${formatMoney(summary.arr)}` },
    { label: "Revenue collected", value: formatMoney(summary.revenueCollected), hint: "In the selected period" },
    { label: "Outstanding", value: formatMoney(summary.outstanding), hint: `${formatMoney(summary.overdueTotal)} overdue`, alert: summary.overdueTotal > 0 },
  ];

  return (
    <>
      <Topbar crumbs={[{ label: "Overview" }, { label: "Finance dashboard" }]} />
      <LiveRefresh seconds={30} />
      <main className="mx-auto w-full max-w-7xl space-y-6 p-4 sm:p-6">
        <FilterBar />

        <section aria-label="Key figures" className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
          {kpis.map((k) => (
            <div key={k.label} className="rounded-xl border border-slate-200 bg-white p-5">
              <p className="text-sm text-slate-500">{k.label}</p>
              <p className={`mt-1 text-2xl font-semibold tabular-nums ${k.alert ? "text-rose-700" : "text-[#1E293B]"}`}>{k.value}</p>
              {k.hint && <p className="mt-1 text-xs text-slate-500">{k.hint}</p>}
            </div>
          ))}
        </section>

        <div className="grid gap-6 lg:grid-cols-3">
          <section aria-label="Overdue aging" className="rounded-xl border border-slate-200 bg-white p-5 lg:col-span-1">
            <h2 className="font-semibold text-[#1E293B]">Overdue by age</h2>
            <ul className="mt-4 space-y-4">
              {summary.aging.map((a) => (
                <li key={a.bucket}>
                  <div className="flex justify-between text-sm">
                    <span className="text-slate-600">{a.bucket} days · {a.count} invoice{a.count === 1 ? "" : "s"}</span>
                    <span className="font-medium tabular-nums">{formatMoney(a.amount)}</span>
                  </div>
                  <div className="mt-1.5 h-2 rounded-full bg-slate-100" role="img" aria-label={`${a.bucket} days: ${formatMoney(a.amount)}`}>
                    <div className="h-2 rounded-full bg-[#6366F1]" style={{ width: `${(a.amount / maxBucket) * 100}%` }} />
                  </div>
                </li>
              ))}
            </ul>
          </section>

          <section aria-label="Overdue invoices" className="rounded-xl border border-slate-200 bg-white lg:col-span-2">
            <div className="flex items-center justify-between p-5 pb-3">
              <h2 className="font-semibold text-[#1E293B]">Overdue invoices</h2>
              <span className="text-sm text-slate-500">{overdue.total.toLocaleString("en-IN")} total</span>
            </div>
            {overdue.rows.length === 0 ? (
              <p className="px-5 pb-8 pt-2 text-sm text-slate-500">Nothing is overdue for these filters.</p>
            ) : (
              <div className="overflow-x-auto">
                <table className="w-full min-w-[560px] text-left text-sm">
                  <thead className="border-y border-slate-200 bg-slate-50 text-slate-600">
                    <tr>
                      <th className="px-5 py-2 font-medium">Invoice</th>
                      <th className="px-3 py-2 font-medium">Customer</th>
                      <th className="px-3 py-2 font-medium">Due</th>
                      <th className="px-5 py-2 text-right font-medium">Balance</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-100">
                    {overdue.rows.map((r) => {
                      const days = Math.floor((Date.now() - r.dueDate.getTime()) / 86_400_000);
                      return (
                        <tr key={r.id} className="hover:bg-slate-50">
                          <td className="px-5 py-3"><Link className="font-medium text-[#6366F1] hover:underline" href={`/invoices/${r.id}`}>{r.number}</Link></td>
                          <td className="px-3 py-3">{r.customer.name}<span className="block text-xs text-slate-500">{r.customer.email}</span></td>
                          <td className="px-3 py-3 text-slate-600">{r.dueDate.toLocaleDateString("en-IN")} <span className="text-rose-700">({days}d late)</span></td>
                          <td className="px-5 py-3 text-right font-medium tabular-nums">{formatMoney(r.total - r.amountPaid)}</td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            )}
            {overdue.pageCount > 1 && (
              <div className="flex items-center justify-between border-t border-slate-200 px-5 py-3 text-sm">
                <PageLink sp={searchParams} page={page - 1} disabled={page <= 1}>Previous</PageLink>
                <span className="text-slate-500">Page {page} of {overdue.pageCount}</span>
                <PageLink sp={searchParams} page={page + 1} disabled={page >= overdue.pageCount}>Next</PageLink>
              </div>
            )}
          </section>
        </div>
      </main>
    </>
  );
}

function PageLink({ sp, page, disabled, children }: { sp: SP; page: number; disabled: boolean; children: string }) {
  const q = new URLSearchParams();
  Object.entries(sp).forEach(([k, v]) => { const s = first(v); if (s && k !== "page") q.set(k, s); });
  q.set("page", String(page));
  if (disabled) return <span className="text-slate-300">{children}</span>;
  return <Link href={`/dashboard?${q}`} className="font-medium text-[#6366F1] hover:underline">{children}</Link>;
}
