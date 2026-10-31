"use server";

import bcrypt from "bcryptjs";
import crypto from "node:crypto";
import fs from "node:fs/promises";
import path from "node:path";
import { redirect } from "next/navigation";
import { Prisma } from "@prisma/client";
import { db } from "@/lib/db";
import { createSession, destroySession } from "@/lib/auth/session";
import { assertRole, ForbiddenError } from "@/lib/auth/rbac";
import { createStaffSchema, forgotSchema, loginSchema, registerSchema, resetSchema } from "@/lib/validation";
import type { Role } from "@/lib/enums";

export type ActionResult<T = undefined> =
  | { ok: true; data?: T }
  | { ok: false; error: string; fieldErrors?: Record<string, string[]> };

const BCRYPT_COST = 12;
const sha256 = (v: string) => crypto.createHash("sha256").update(v).digest("hex");

export async function registerAction(input: unknown): Promise<ActionResult> {
  const parsed = registerSchema.safeParse(input);
  if (!parsed.success) return { ok: false, error: "Check the highlighted fields", fieldErrors: parsed.error.flatten().fieldErrors };

  const { name, email, password } = parsed.data;
  try {
    const user = await db.user.create({
      data: { name, email, passwordHash: await bcrypt.hash(password, BCRYPT_COST), role: "CUSTOMER" },
    });
    await createSession({ id: user.id, role: "CUSTOMER", name: user.name, email: user.email });
    return { ok: true };
  } catch (e) {
    // The unique index is the source of truth for "one account per email" (race-safe).
    if (e instanceof Prisma.PrismaClientKnownRequestError && e.code === "P2002") {
      return { ok: false, error: "An account with this email already exists", fieldErrors: { email: ["Email already registered"] } };
    }
    throw e;
  }
}

export async function loginAction(input: unknown): Promise<ActionResult> {
  const parsed = loginSchema.safeParse(input);
  if (!parsed.success) return { ok: false, error: "Enter a valid email and password" };

  const user = await db.user.findUnique({ where: { email: parsed.data.email } });
  // Compare against a dummy hash when the user is missing so response time doesn't leak existence.
  const hash = user?.passwordHash ?? "$2a$12$C6UzMDM.H6dfI/f/IKcEeO5G7m7R9wZ6dQpYQ1Q5mO5M5s1x5x5xa";
  const valid = await bcrypt.compare(parsed.data.password, hash);
  if (!user || !valid || !user.isActive) return { ok: false, error: "Email or password is incorrect" };

  await createSession({ id: user.id, role: user.role as Role, name: user.name, email: user.email });
  return { ok: true };
}

export async function logoutAction(): Promise<void> {
  destroySession();
  redirect("/login");
}

/**
 * Always returns ok to avoid revealing which emails are registered.
 * Local-first: the reset link is written to ./.outbox/ (and logged in dev). Swap `deliver` for SMTP in prod.
 */
export async function requestPasswordResetAction(input: unknown): Promise<ActionResult> {
  const parsed = forgotSchema.safeParse(input);
  if (!parsed.success) return { ok: false, error: "Enter a valid email address" };

  const user = await db.user.findUnique({ where: { email: parsed.data.email } });
  if (user && user.isActive) {
    const raw = crypto.randomBytes(32).toString("hex");
    await db.passwordResetToken.create({
      data: { userId: user.id, tokenHash: sha256(raw), expiresAt: new Date(Date.now() + 30 * 60_000) },
    });
    await deliver(user.email, `${process.env.APP_URL ?? "http://localhost:3000"}/reset-password?token=${raw}`);
  }
  return { ok: true };
}

async function deliver(to: string, link: string): Promise<void> {
  if (process.env.SMTP_URL) {
    // const nodemailer = await import("nodemailer"); ...send via SMTP here
  }
  const dir = path.join(process.cwd(), ".outbox");
  await fs.mkdir(dir, { recursive: true });
  await fs.writeFile(path.join(dir, `${Date.now()}-reset.txt`), `To: ${to}\nSubject: Reset your Brightloop password\n\n${link}\n`);
  if (process.env.NODE_ENV !== "production") console.info(`[outbox] reset link for ${to}: ${link}`);
}

export async function resetPasswordAction(input: unknown): Promise<ActionResult> {
  const parsed = resetSchema.safeParse(input);
  if (!parsed.success) return { ok: false, error: "Check the highlighted fields", fieldErrors: parsed.error.flatten().fieldErrors };

  const record = await db.passwordResetToken.findUnique({ where: { tokenHash: sha256(parsed.data.token) } });
  if (!record || record.usedAt || record.expiresAt < new Date()) return { ok: false, error: "This reset link is invalid or has expired" };

  await db.$transaction([
    db.user.update({ where: { id: record.userId }, data: { passwordHash: await bcrypt.hash(parsed.data.password, BCRYPT_COST) } }),
    db.passwordResetToken.update({ where: { id: record.id }, data: { usedAt: new Date() } }),
  ]);
  return { ok: true };
}

/** Only ADMIN can create STAFF accounts. */
export async function createStaffAction(input: unknown): Promise<ActionResult> {
  try {
    const admin = await assertRole("ADMIN");
    const parsed = createStaffSchema.safeParse(input);
    if (!parsed.success) return { ok: false, error: "Check the highlighted fields", fieldErrors: parsed.error.flatten().fieldErrors };
    const { name, email, password } = parsed.data;
    await db.user.create({
      data: {
        name,
        email,
        passwordHash: await bcrypt.hash(password, BCRYPT_COST),
        role: "STAFF",
        createdById: admin.id,
      },
    });
    return { ok: true };
  } catch (e) {
    if (e instanceof ForbiddenError) return { ok: false, error: e.message };
    if (e instanceof Prisma.PrismaClientKnownRequestError && e.code === "P2002") return { ok: false, error: "Email already registered", fieldErrors: { email: ["Email already registered"] } };
    throw e;
  }
}
