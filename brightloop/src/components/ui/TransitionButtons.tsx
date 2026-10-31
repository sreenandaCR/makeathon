"use client";

import { useRouter } from "next/navigation";
import { useState, useTransition } from "react";
import { pauseAction, renewAction, transitionAction } from "@/actions/subscriptions";
import type { ActionResult } from "@/actions/auth";

const LABEL: Record<string, string> = {
  QUOTATION: "Send as quotation",
  DRAFT: "Back to draft",
  CONFIRMED: "Confirm",
  ACTIVE: "Activate",
  CLOSED: "Close subscription",
};

export function TransitionButtons(props: {
  id: string;
  next: string[];
  canPause: boolean;
  paused: boolean;
  canRenew: boolean;
}) {
  const router = useRouter();
  const [pending, start] = useTransition();
  const [error, setError] = useState<string | null>(null);

  const run = (fn: () => Promise<ActionResult<{ id: string }> | ActionResult>, after?: (r: ActionResult<{ id: string }>) => void) =>
    start(async () => {
      setError(null);
      const r = (await fn()) as ActionResult<{ id: string }>;
      if (!r.ok) return setError(r.error);
      after ? after(r) : router.refresh();
    });

  const base = "rounded-lg px-4 py-2 text-sm font-medium outline-none focus-visible:ring-2 focus-visible:ring-[#6366F1] disabled:opacity-50";
  return (
    <div>
      <div className="flex flex-wrap gap-2">
        {props.next.map((to) => (
          <button
            key={to}
            disabled={pending}
            onClick={() => {
              if (to === "CLOSED" && !confirm("Close this subscription? This can't be undone.")) return;
              run(() => transitionAction(props.id, to));
            }}
            className={`${base} ${to === "CLOSED" || to === "DRAFT" ? "border border-slate-300 bg-white text-slate-700 hover:bg-slate-50" : "bg-[#6366F1] text-white hover:bg-indigo-500"}`}
          >
            {LABEL[to] ?? to}
          </button>
        ))}
        {props.canPause && (
          <button disabled={pending} onClick={() => run(() => pauseAction(props.id, !props.paused))} className={`${base} border border-slate-300 bg-white text-slate-700 hover:bg-slate-50`}>
            {props.paused ? "Resume billing" : "Pause billing"}
          </button>
        )}
        {props.canRenew && (
          <button disabled={pending} onClick={() => run(() => renewAction(props.id), (r) => r.ok && r.data && router.push(`/subscriptions/${r.data.id}`))} className={`${base} border border-slate-300 bg-white text-slate-700 hover:bg-slate-50`}>
            Renew
          </button>
        )}
      </div>
      {error && <p role="alert" className="mt-2 text-sm text-rose-700">{error}</p>}
    </div>
  );
}
