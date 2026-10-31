import { describe, expect, it } from "vitest";
import type { DiscountRule } from "@prisma/client";
import { computeInvoice, eligibleRules, type PricedLine } from "../src/lib/billing/pricing";
import { addPeriod } from "../src/lib/dates";

const now = new Date("2026-11-01T00:00:00Z");
const rule = (o: Partial<DiscountRule>): DiscountRule => ({
  id: "r1", name: "r", type: "PERCENT", value: 1000, scope: "SUBSCRIPTION", productId: null,
  minAmount: null, minQuantity: null, startsAt: new Date("2026-01-01"), endsAt: new Date("2027-01-01"),
  maxRedemptions: null, redemptionCount: 0, isActive: true, createdAt: now, ...o,
});
const lines: PricedLine[] = [
  { productId: "a", description: "A", quantity: 10, unitPrice: 10000, taxRateBps: 1800 },
  { productId: "b", description: "B", quantity: 1, unitPrice: 56000, taxRateBps: 0 },
];

describe("pricing", () => {
  it("applies tax on the discounted net and totals add up exactly", () => {
    const c = computeInvoice(lines, [rule({ type: "FLAT", value: 10001 })]);
    expect(c.discountTotal).toBe(10001);
    expect(c.total).toBe(c.subtotal - c.discountTotal + c.taxTotal);
  });
  it("respects minimum quantity, expiry and redemption limits", () => {
    expect(eligibleRules([rule({ minQuantity: 50 })], lines, now)).toHaveLength(0);
    expect(eligibleRules([rule({ endsAt: new Date("2026-10-01") })], lines, now)).toHaveLength(0);
    expect(eligibleRules([rule({ maxRedemptions: 3, redemptionCount: 3 })], lines, now)).toHaveLength(0);
    expect(eligibleRules([rule({})], lines, now)).toHaveLength(1);
  });
  it("scopes product discounts to that product only", () => {
    const c = computeInvoice(lines, [rule({ scope: "PRODUCT", productId: "b" })]);
    expect(c.lines[0].discount).toBe(0);
    expect(c.lines[1].discount).toBe(5600);
  });
  it("never discounts below zero", () => {
    expect(computeInvoice(lines, [rule({ type: "FLAT", value: 99_999_999 })]).total).toBe(0);
  });
});

describe("dates", () => {
  it("clamps month-end", () => {
    expect(addPeriod(new Date("2026-01-31T00:00:00Z"), "MONTHLY").toISOString()).toBe("2026-02-28T00:00:00.000Z");
  });
});
