import { redirect } from "next/navigation";
import type { Role } from "../enums";
import { readSession, type SessionUser } from "./session";

export class ForbiddenError extends Error {
  constructor(message = "You do not have permission to do this") {
    super(message);
    this.name = "ForbiddenError";
  }
}

/** Route prefix -> roles allowed. Used by middleware and the sidebar. */
export const ROUTE_ACCESS: Array<{ prefix: string; roles: Role[] }> = [
  { prefix: "/admin", roles: ["ADMIN"] },
  { prefix: "/products", roles: ["ADMIN", "STAFF"] },
  { prefix: "/plans", roles: ["ADMIN", "STAFF"] },
  { prefix: "/discounts", roles: ["ADMIN"] },
  { prefix: "/dashboard", roles: ["ADMIN", "STAFF"] },
  { prefix: "/subscriptions", roles: ["ADMIN", "STAFF", "CUSTOMER"] },
  { prefix: "/invoices", roles: ["ADMIN", "STAFF", "CUSTOMER"] },
  { prefix: "/payments", roles: ["ADMIN", "STAFF", "CUSTOMER"] },
];

export function canAccess(role: Role, pathname: string): boolean {
  const rule = ROUTE_ACCESS.find((r) => pathname === r.prefix || pathname.startsWith(r.prefix + "/"));
  return rule ? rule.roles.includes(role) : true;
}

/** For server components / pages: redirects when unauthenticated. */
export async function requireUser(...roles: Role[]): Promise<SessionUser> {
  const user = await readSession();
  if (!user) redirect("/login");
  if (roles.length && !roles.includes(user.role)) redirect("/forbidden");
  return user;
}

/** For server actions: throws instead of redirecting, so the caller can return a typed error. */
export async function assertRole(...roles: Role[]): Promise<SessionUser> {
  const user = await readSession();
  if (!user) throw new ForbiddenError("Sign in to continue");
  if (roles.length && !roles.includes(user.role)) throw new ForbiddenError();
  return user;
}

/** Customers may only touch their own records. Staff/admin may touch any. */
export function assertOwnerOrStaff(user: SessionUser, ownerId: string): void {
  if (user.role === "CUSTOMER" && user.id !== ownerId) throw new ForbiddenError();
}
