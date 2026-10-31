import type { ReactNode } from "react";
import { requireUser } from "@/lib/auth/rbac";
import { navFor } from "@/components/layout/nav";
import { Sidebar } from "@/components/layout/Sidebar";

export default async function AppLayout({ children }: { children: ReactNode }) {
  const user = await requireUser();
  return (
    <div className="min-h-screen bg-slate-50 lg:flex">
      <div className="lg:w-64 lg:shrink-0">
        <div className="lg:fixed lg:inset-y-0 lg:w-64">
          <Sidebar groups={navFor(user.role)} userName={user.name} role={user.role} />
        </div>
      </div>
      <div className="flex min-w-0 flex-1 flex-col">{children}</div>
    </div>
  );
}
