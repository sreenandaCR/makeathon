"use server";

import { revalidatePath } from "next/cache";
import { assertRole, ForbiddenError } from "@/lib/auth/rbac";
import { renewSubscription, setPaused, transitionSubscription, TransitionError } from "@/lib/subscription-state";
import { SUB_STATUSES, type SubStatus } from "@/lib/enums";
import type { ActionResult } from "./auth";

export async function transitionAction(id: string, to: string): Promise<ActionResult> {
  try {
    await assertRole("ADMIN", "STAFF");
    if (!(SUB_STATUSES as readonly string[]).includes(to)) return { ok: false, error: "Unknown status" };
    await transitionSubscription(id, to as SubStatus);
    revalidatePath(`/subscriptions/${id}`);
    return { ok: true };
  } catch (e) {
    return fail(e);
  }
}

export async function pauseAction(id: string, paused: boolean): Promise<ActionResult> {
  try {
    await assertRole("ADMIN", "STAFF");
    await setPaused(id, paused);
    revalidatePath(`/subscriptions/${id}`);
    return { ok: true };
  } catch (e) {
    return fail(e);
  }
}

export async function renewAction(id: string): Promise<ActionResult<{ id: string }>> {
  try {
    await assertRole("ADMIN", "STAFF");
    const next = await renewSubscription(id);
    return { ok: true, data: { id: next.id } };
  } catch (e) {
    return fail(e);
  }
}

function fail(e: unknown): { ok: false; error: string } {
  if (e instanceof ForbiddenError || e instanceof TransitionError) return { ok: false, error: e.message };
  console.error(e);
  return { ok: false, error: "Something went wrong. Try again." };
}
