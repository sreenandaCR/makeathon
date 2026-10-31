# Contributing to Brightloop

## Branch model

| Branch | Purpose | Deploys to | Rules |
|---|---|---|---|
| `main` | Production-ready, tagged releases | Production | Protected. PR only, from `staging`. Squash-free merge commit. 1+ approval, CI green. |
| `staging` | Integration + QA | Staging | Protected. PR only, from `feature/*`, `fix/*`. 1+ approval, CI green. |
| `feature/<ticket>-<slug>` | New work | — | Branch from `staging`. Delete after merge. |
| `fix/<ticket>-<slug>` | Bug fixes | — | Branch from `staging`. |
| `hotfix/<slug>` | Urgent prod fix | — | Branch from `main`; PR to `main`, then merge back to `staging`. |
| `release/x.y.z` | Optional stabilisation | — | Branch from `staging`, merge to `main`, tag `vX.Y.Z`. |

Keep branches short-lived (under ~3 days). Rebase on `staging` before opening a PR: `git fetch && git rebase origin/staging`.

## Commit messages (Conventional Commits)

```
<type>(<scope>): <imperative summary, <=72 chars>

[body: why, not what]
[footer: BREAKING CHANGE: ... | Closes #123]
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`, `revert`.
Scopes: `auth`, `catalog`, `subscriptions`, `billing`, `discounts`, `payments`, `dashboard`, `ui`, `db`.

Examples:
- `feat(billing): claim billing period atomically to prevent double invoicing`
- `fix(discounts): stop applying rules after maxRedemptions is reached`
- `feat(db)!: store money as integer minor units` (with `BREAKING CHANGE:` footer)

Commit hooks (husky + commitlint) reject non-conforming messages.

## Workflow

1. Pick or create an issue; assign yourself.
2. `git switch staging && git pull && git switch -c feature/123-discount-rules`
3. Commit small, logical units. Run `npm run lint && npm run typecheck && npm test` before pushing.
4. Schema change? `npx prisma migrate dev --name <what>` and commit the generated `prisma/migrations/*`. Never edit an applied migration.
5. Open a PR into `staging` using the template. Link the issue, add screenshots for UI, note any migration.
6. Reviewer checks: tests for new logic (especially billing/discount math), Zod validation on every new input, RBAC check on every new action, no secrets.
7. Merge with the "Merge commit" or "Rebase" button once approved and green. Delete the branch.
8. Release: PR `staging` -> `main`, tag `vX.Y.Z`, generate the changelog from Conventional Commits.

## Definition of done

- Typed, linted, tested; no `any` without a comment.
- Server-side validation and role check present.
- Migrations included and reversible in dev.
- Works offline against local SQLite (`npm run dev` with no network).
- Docs updated if behaviour or setup changed.

## Repo layout

```
prisma/            schema + migrations + seed.ts
src/app/           routes (App Router): (auth), (app)/dashboard, subscriptions, invoices ...
src/actions/       server actions (thin: auth check -> validate -> call lib)
src/lib/           domain logic: billing/, subscription-state.ts, analytics.ts, auth/
src/components/    layout/, dashboard/, ui/
tests/             unit (pricing, dates, state machine) + integration (billing idempotency)
.github/           workflows/ci.yml, PULL_REQUEST_TEMPLATE.md
```
