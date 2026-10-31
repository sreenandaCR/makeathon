"use client";

import { useRouter } from "next/navigation";
import { useEffect } from "react";

/**
 * Local-first "real-time": re-runs the server component every `seconds` while the tab is visible,
 * so billing-job status changes and new payments show up without a manual reload. No cloud service needed.
 */
export function LiveRefresh({ seconds = 15 }: { seconds?: number }) {
  const router = useRouter();
  useEffect(() => {
    const tick = () => {
      if (document.visibilityState === "visible" && navigator.onLine) router.refresh();
    };
    const id = setInterval(tick, seconds * 1000);
    document.addEventListener("visibilitychange", tick);
    return () => {
      clearInterval(id);
      document.removeEventListener("visibilitychange", tick);
    };
  }, [router, seconds]);
  return null;
}
