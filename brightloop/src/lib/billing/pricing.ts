import type { DiscountRule } from "@prisma/client";

export interface PricedLine {
  productId: string;
  description: string;
  quantity: number;
  unitPrice: number;
  taxRateBps: number;
}

export interface ComputedLine extends PricedLine {
  gross: number;
  discount: number;
  taxAmount: number;
  total: number;
}

export interface AppliedDiscount {
  ruleId: string;
  amount: number;
}

const pct = (amount: number, bps: number) => Math.round((amount * bps) / 10_000);

/** Pure: which rules' conditions are satisfied for these lines? */
export function eligibleRules(rules: DiscountRule[], lines: PricedLine[], at: Date): DiscountRule[] {
  const subtotal = lines.reduce((s, l) => s + l.quantity * l.unitPrice, 0);
  const qty = lines.reduce((s, l) => s + l.quantity, 0);

  return rules.filter((r) => {
    if (!r.isActive || r.startsAt > at || r.endsAt < at) return false;
    if (r.maxRedemptions != null && r.redemptionCount >= r.maxRedemptions) return false;

    const scoped = r.scope === "PRODUCT" ? lines.filter((l) => l.productId === r.productId) : lines;
    if (scoped.length === 0) return false;
    const scopedAmount = r.scope === "PRODUCT" ? scoped.reduce((s, l) => s + l.quantity * l.unitPrice, 0) : subtotal;
    const scopedQty = r.scope === "PRODUCT" ? scoped.reduce((s, l) => s + l.quantity, 0) : qty;

    if (r.minAmount != null && scopedAmount < r.minAmount) return false;
    if (r.minQuantity != null && scopedQty < r.minQuantity) return false;
    return true;
  });
}

/**
 * Pure: applies reserved rules to lines, then tax on the discounted net.
 * Discounts never push a line below zero; a flat subscription discount is split pro-rata
 * with the rounding remainder landing on the last eligible line so the parts sum exactly.
 */
export function computeInvoice(lines: PricedLine[], rules: DiscountRule[]) {
  const computed: ComputedLine[] = lines.map((l) => ({
    ...l,
    gross: l.quantity * l.unitPrice,
    discount: 0,
    taxAmount: 0,
    total: 0,
  }));
  const applied: AppliedDiscount[] = [];

  for (const rule of rules) {
    const targets = computed.filter((l) => (rule.scope === "PRODUCT" ? l.productId === rule.productId : true));
    const base = targets.reduce((s, l) => s + (l.gross - l.discount), 0);
    if (base <= 0) continue;

    const wanted = rule.type === "PERCENT" ? pct(base, rule.value) : Math.min(rule.value, base);
    let remaining = Math.min(wanted, base);
    let given = 0;
    targets.forEach((l, i) => {
      const room = l.gross - l.discount;
      const share = i === targets.length - 1 ? remaining : Math.min(room, Math.floor((wanted * room) / base));
      const take = Math.min(share, room, remaining);
      l.discount += take;
      remaining -= take;
      given += take;
    });
    if (given > 0) applied.push({ ruleId: rule.id, amount: given });
  }

  for (const l of computed) {
    const net = l.gross - l.discount;
    l.taxAmount = pct(net, l.taxRateBps);
    l.total = net + l.taxAmount;
  }

  const subtotal = computed.reduce((s, l) => s + l.gross, 0);
  const discountTotal = computed.reduce((s, l) => s + l.discount, 0);
  const taxTotal = computed.reduce((s, l) => s + l.taxAmount, 0);
  return { lines: computed, applied, subtotal, discountTotal, taxTotal, total: subtotal - discountTotal + taxTotal };
}
