import { NextResponse, type NextRequest } from "next/server";
import { readSession } from "./lib/auth/session";
import { canAccess } from "./lib/auth/rbac";

const PUBLIC = ["/login", "/register", "/forgot-password", "/reset-password", "/forbidden"];

export async function middleware(req: NextRequest) {
  const { pathname } = req.nextUrl;
  if (PUBLIC.some((p) => pathname.startsWith(p))) return NextResponse.next();

  const user = await readSession(req.cookies.get("bl_session")?.value);
  if (!user) {
    const url = new URL("/login", req.url);
    url.searchParams.set("next", pathname);
    return NextResponse.redirect(url);
  }
  if (!canAccess(user.role, pathname)) return NextResponse.redirect(new URL("/forbidden", req.url));
  return NextResponse.next();
}

export const config = { matcher: ["/((?!_next|api/cron|favicon.ico|.*\\..*).*)"] };
