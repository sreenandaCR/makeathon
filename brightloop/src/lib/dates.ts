import type { Frequency } from "./enums";

/** Adds one billing period in UTC. Monthly/yearly clamp to month-end (Jan 31 + 1 month = Feb 28/29). */
export function addPeriod(date: Date, frequency: Frequency, count = 1): Date {
  const d = new Date(date.getTime());
  switch (frequency) {
    case "DAILY":
      d.setUTCDate(d.getUTCDate() + count);
      return d;
    case "WEEKLY":
      d.setUTCDate(d.getUTCDate() + 7 * count);
      return d;
    case "MONTHLY":
    case "YEARLY": {
      const months = frequency === "MONTHLY" ? count : 12 * count;
      const day = d.getUTCDate();
      d.setUTCDate(1);
      d.setUTCMonth(d.getUTCMonth() + months);
      const lastDay = new Date(Date.UTC(d.getUTCFullYear(), d.getUTCMonth() + 1, 0)).getUTCDate();
      d.setUTCDate(Math.min(day, lastDay));
      return d;
    }
  }
}

export const addDays = (d: Date, n: number) => new Date(d.getTime() + n * 86_400_000);
