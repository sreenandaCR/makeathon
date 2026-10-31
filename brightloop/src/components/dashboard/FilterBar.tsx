"use client";

import { usePathname, useRouter, useSearchParams } from "next/navigation";
import { useTransition } from "react";
import { FREQUENCIES } from "@/lib/enums";

const RANGES = [
  { value: "30d", label: "Last 30 days" },
  { value: "90d", label: "Last 90 days" },
  { value: "ytd", label: "Year to date" },
  { value: "all", label: "All time" },
];
const AGING = ["all", "0-30", "31-60", "61-90", "90+"];

/** Filters live in the URL so views are shareable and the server component re-queries on change. */
export function FilterBar() {
  const router = useRouter();
  const pathname = usePathname();
  const params = useSearchParams();
  const [pending, start] = useTransition();

  const set = (key: string, value: string) => {
    const next = new URLSearchParams(params.toString());
    if (!value || value === "all") next.delete(key);
    else next.set(key, value);
    next.delete("page");
    start(() => router.replace(`${pathname}?${next.toString()}`, { scroll: false }));
  };

  const select = "rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm outline-none focus:border-[#6366F1] focus:ring-2 focus:ring-[#6366F1]/30";
  return (
    <div className="flex flex-wrap items-end gap-3" aria-busy={pending}>
      <label className="text-sm text-slate-600">
        <span className="mb-1 block">Period</span>
        <select className={select} value={params.get("range") ?? "30d"} onChange={(e) => set("range", e.target.value)}>
          {RANGES.map((r) => <option key={r.value} value={r.value}>{r.label}</option>)}
        </select>
      </label>
      <label className="text-sm text-slate-600">
        <span className="mb-1 block">Billing frequency</span>
        <select className={select} value={params.get("frequency") ?? "all"} onChange={(e) => set("frequency", e.target.value)}>
          <option value="all">All</option>
          {FREQUENCIES.map((f) => <option key={f} value={f}>{f.charAt(0) + f.slice(1).toLowerCase()}</option>)}
        </select>
      </label>
      <label className="text-sm text-slate-600">
        <span className="mb-1 block">Overdue age (days)</span>
        <select className={select} value={params.get("aging") ?? "all"} onChange={(e) => set("aging", e.target.value)}>
          {AGING.map((a) => <option key={a} value={a}>{a === "all" ? "Any" : a}</option>)}
        </select>
      </label>
      {pending && <span className="pb-2 text-sm text-slate-500">Updating…</span>}
    </div>
  );
}
