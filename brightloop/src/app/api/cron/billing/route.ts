import { NextResponse } from "next/server";
import { runBillingCycle } from "@/lib/billing/processor";

export const dynamic = "force-dynamic";

/** Trigger from OS cron / node-cron / systemd timer:  curl -H "x-cron-secret: $CRON_SECRET" localhost:3000/api/cron/billing */
export async function POST(req: Request) {
  if (req.headers.get("x-cron-secret") !== process.env.CRON_SECRET) {
    return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
  }
  const report = await runBillingCycle();
  return NextResponse.json(report);
}
