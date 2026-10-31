# Brightloop Subscription Management

Next.js 14 (App Router) · TypeScript · Prisma · SQLite (local) / PostgreSQL (prod) · Tailwind · Zod. Runs fully offline.

## Run locally
```
cp .env.example .env
npm install
npx prisma migrate dev --name init
npm run dev
```
Billing job: `npm run billing:run` (or `POST /api/cron/billing` with `x-cron-secret`). Schedule it daily with OS cron.
Password-reset emails are written to `./.outbox/` unless `SMTP_URL` is set.

## Architecture
- **Thin actions, fat domain**: `src/actions/*` do auth -> validate -> call `src/lib/*`.
- **Money** is integer minor units; the Odoo variant's +560 is stored as 56000.
- **No double or skipped billing**: period claim via compare-and-set on `nextBillingDate` + `@@unique([subscriptionId, periodStart])`; missed cycles are caught up one period at a time.
- **Discounts** are evaluated at invoice time (dates, min amount/qty, scope) with an atomic redemption reservation, so they auto-apply and expire cleanly.
- **Dashboard speed**: every metric is one indexed aggregate run in parallel; MRR uses the denormalised `Subscription.recurringTotal` (keep it updated when lines change).
- **Live updates**: `LiveRefresh` re-renders server components on an interval, no external service.
- **RBAC** is enforced three times: middleware (routes), server components/actions (`requireUser`/`assertRole`), and row ownership (`assertOwnerOrStaff`).

## Not included here (follow the same patterns)
CRUD screens for products, plans, discounts, templates and invoices; seed script; invoice print view (`@media print` page); PDF/CSV export; login/register forms (use `registerSchema` etc. with react-hook-form).
