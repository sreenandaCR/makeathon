/** Formats integer minor units (paise/cents) for display. */
export function formatMoney(minor: number, currency = process.env.NEXT_PUBLIC_CURRENCY ?? "INR"): string {
  return new Intl.NumberFormat("en-IN", { style: "currency", currency }).format(minor / 100);
}
