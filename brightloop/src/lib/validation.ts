import { z } from "zod";
import { FREQUENCIES, PAYMENT_METHODS, PAYMENT_TERMS } from "./enums";

/** Shared by client forms (react-hook-form + zodResolver) and server actions. */
export const passwordSchema = z
  .string()
  .min(9, "Password must be more than 8 characters")
  .regex(/[A-Z]/, "Include at least one uppercase letter")
  .regex(/[a-z]/, "Include at least one lowercase letter")
  .regex(/[^A-Za-z0-9]/, "Include at least one special character");

export const emailSchema = z.string().trim().toLowerCase().email("Enter a valid email address");

export const registerSchema = z
  .object({
    name: z.string().trim().min(2, "Enter your name").max(80),
    email: emailSchema,
    password: passwordSchema,
    confirmPassword: z.string(),
  })
  .refine((v) => v.password === v.confirmPassword, {
    path: ["confirmPassword"],
    message: "Passwords do not match",
  });

export const loginSchema = z.object({ email: emailSchema, password: z.string().min(1, "Enter your password") });
export const forgotSchema = z.object({ email: emailSchema });
export const resetSchema = z.object({ token: z.string().min(20), password: passwordSchema });
export const createStaffSchema = z.object({
  name: z.string().trim().min(2).max(80),
  email: emailSchema,
  password: passwordSchema,
});

const money = z.number().int("Use whole minor units").nonnegative("Cannot be negative").max(1_000_000_000);

export const variantSchema = z.object({
  attribute: z.string().trim().min(1),
  value: z.string().trim().min(1),
  priceModifier: z.number().int().min(-1_000_000_000).max(1_000_000_000), // Odoo brand: 56000 = +560.00
});

export const productSchema = z
  .object({
    name: z.string().trim().min(2).max(120),
    type: z.string().trim().min(2).max(40),
    salesPrice: money,
    costPrice: money,
    notes: z.string().max(2000).optional(),
    variants: z.array(variantSchema).max(50).default([]),
  })
  .superRefine((p, ctx) => {
    const seen = new Set<string>();
    p.variants.forEach((v, i) => {
      const k = `${v.attribute}:${v.value}`.toLowerCase();
      if (seen.has(k)) ctx.addIssue({ code: "custom", path: ["variants", i, "value"], message: "Duplicate variant" });
      seen.add(k);
    });
  });

export const planSchema = z
  .object({
    name: z.string().trim().min(2).max(120),
    price: money,
    frequency: z.enum(FREQUENCIES),
    minQuantity: z.number().int().min(1).max(100_000),
    availableFrom: z.coerce.date(),
    availableTo: z.coerce.date().optional(),
    autoClose: z.boolean(),
    pausable: z.boolean(),
    closable: z.boolean(),
    renewable: z.boolean(),
  })
  .refine((p) => !p.availableTo || p.availableFrom < p.availableTo, {
    path: ["availableTo"],
    message: "End date must be after the start date",
  })
  .refine((p) => !p.autoClose || !!p.availableTo, { path: ["autoClose"], message: "Auto-close needs an end date" });

export const subscriptionLineSchema = z.object({
  productId: z.string().cuid(),
  variantId: z.string().cuid().optional(),
  quantity: z.number().int().min(1).max(100_000),
});

const termKeys = Object.keys(PAYMENT_TERMS) as [keyof typeof PAYMENT_TERMS, ...(keyof typeof PAYMENT_TERMS)[]];

export const subscriptionSchema = z
  .object({
    customerId: z.string().cuid(),
    planId: z.string().cuid(),
    startDate: z.coerce.date(),
    expiryDate: z.coerce.date().optional(),
    paymentTerms: z.enum(termKeys),
    lines: z.array(subscriptionLineSchema).min(1, "Add at least one line"),
  })
  .refine((s) => !s.expiryDate || s.startDate < s.expiryDate, {
    path: ["expiryDate"],
    message: "Expiry must be after the start date",
  });

export const discountSchema = z
  .object({
    name: z.string().trim().min(2).max(120),
    type: z.enum(["FLAT", "PERCENT"]),
    value: z.number().int().positive(), // FLAT: minor units, PERCENT: basis points
    scope: z.enum(["PRODUCT", "SUBSCRIPTION"]),
    productId: z.string().cuid().optional(),
    minAmount: z.number().int().nonnegative().optional(),
    minQuantity: z.number().int().positive().optional(),
    startsAt: z.coerce.date(),
    endsAt: z.coerce.date(),
    maxRedemptions: z.number().int().positive().optional(),
  })
  .refine((d) => d.startsAt < d.endsAt, { path: ["endsAt"], message: "End date must be after the start date" })
  .refine((d) => d.type !== "PERCENT" || d.value <= 10000, { path: ["value"], message: "Percentage cannot exceed 100%" })
  .refine((d) => d.scope !== "PRODUCT" || !!d.productId, { path: ["productId"], message: "Choose a product" });

export const paymentSchema = z.object({
  invoiceId: z.string().cuid(),
  method: z.enum(PAYMENT_METHODS),
  amount: z.number().int().positive("Amount must be greater than zero"),
  paidAt: z.coerce.date().refine((d) => d.getTime() <= Date.now() + 86_400_000, "Payment date cannot be in the future"),
  reference: z.string().max(120).optional(),
  note: z.string().max(500).optional(),
});

export const dashboardFilterSchema = z.object({
  range: z.enum(["30d", "90d", "ytd", "all"]).default("30d"),
  frequency: z.enum(FREQUENCIES).optional(),
  aging: z.enum(["all", "0-30", "31-60", "61-90", "90+"]).default("all"),
});
export type DashboardFilters = z.infer<typeof dashboardFilterSchema>;
