import { SignJWT, jwtVerify } from "jose";
import { cookies } from "next/headers";
import type { Role } from "../enums";

const COOKIE = "bl_session";
const secret = () => {
  const s = process.env.AUTH_SECRET;
  if (!s || s.length < 32) throw new Error("AUTH_SECRET must be set (32+ chars)");
  return new TextEncoder().encode(s);
};

export interface SessionUser {
  id: string;
  role: Role;
  name: string;
  email: string;
}

export async function createSession(user: SessionUser): Promise<void> {
  const token = await new SignJWT({ ...user })
    .setProtectedHeader({ alg: "HS256" })
    .setSubject(user.id)
    .setIssuedAt()
    .setExpirationTime("7d")
    .sign(secret());
  cookies().set(COOKIE, token, {
    httpOnly: true,
    sameSite: "lax",
    secure: process.env.NODE_ENV === "production",
    path: "/",
    maxAge: 60 * 60 * 24 * 7,
  });
}

export async function readSession(token?: string): Promise<SessionUser | null> {
  const value = token ?? cookies().get(COOKIE)?.value;
  if (!value) return null;
  try {
    const { payload } = await jwtVerify(value, secret());
    return {
      id: String(payload.sub),
      role: payload.role as Role,
      name: String(payload.name),
      email: String(payload.email),
    };
  } catch {
    return null;
  }
}

export function destroySession(): void {
  cookies().delete(COOKIE);
}
