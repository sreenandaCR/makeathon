export const ROLES = ["ADMIN", "STAFF", "CUSTOMER"] as const;
export type Role = (typeof ROLES)[number];

export const FREQUENCIES = ["DAILY", "WEEKLY", "MONTHLY", "YEARLY"] as const;
export type Frequency = (typeof FREQUENCIES)[number];

export const SUB_STATUSES = ["DRAFT", "QUOTATION", "CONFIRMED", "ACTIVE", "CLOSED"] as const;
export type SubStatus = (typeof SUB_STATUSES)[number];

export const INVOICE_STATUSES = ["DRAFT", "CONFIRMED", "PAID", "CANCELLED"] as const;
export type InvoiceStatus = (typeof INVOICE_STATUSES)[number];

export const PAYMENT_METHODS = ["CASH", "BANK_TRANSFER", "CARD", "UPI", "CHEQUE"] as const;
export const PAYMENT_TERMS = { NET_0: 0, NET_15: 15, NET_30: 30 } as const;
export type PaymentTerms = keyof typeof PAYMENT_TERMS;
