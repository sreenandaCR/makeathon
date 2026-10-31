const TONES: Record<string, string> = {
  DRAFT: "bg-slate-100 text-slate-700 ring-slate-200",
  QUOTATION: "bg-amber-50 text-amber-800 ring-amber-200",
  CONFIRMED: "bg-sky-50 text-sky-800 ring-sky-200",
  ACTIVE: "bg-emerald-50 text-emerald-800 ring-emerald-200",
  PAUSED: "bg-violet-50 text-violet-800 ring-violet-200",
  CLOSED: "bg-slate-200 text-slate-600 ring-slate-300",
  PAID: "bg-emerald-50 text-emerald-800 ring-emerald-200",
  CANCELLED: "bg-rose-50 text-rose-800 ring-rose-200",
  OVERDUE: "bg-rose-50 text-rose-800 ring-rose-200",
};

const LABELS: Record<string, string> = { QUOTATION: "Quotation", CONFIRMED: "Confirmed", ACTIVE: "Active", DRAFT: "Draft", CLOSED: "Closed", PAID: "Paid", CANCELLED: "Cancelled", PAUSED: "Paused", OVERDUE: "Overdue" };

export function StatusBadge({ status }: { status: string }) {
  return (
    <span className={`inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-medium ring-1 ring-inset ${TONES[status] ?? TONES.DRAFT}`}>
      {LABELS[status] ?? status}
    </span>
  );
}
