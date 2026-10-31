"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { useEffect, useState } from "react";
import * as Icons from "lucide-react";
import type { NavGroup } from "./nav";

export function Sidebar({ groups, userName, role }: { groups: NavGroup[]; userName: string; role: string }) {
  const pathname = usePathname();
  const [open, setOpen] = useState(false);
  useEffect(() => setOpen(false), [pathname]);

  const content = (
    <nav aria-label="Main" className="flex h-full flex-col bg-[#1E293B] text-slate-300">
      <div className="flex h-16 items-center gap-2 px-5 text-white">
        <span className="grid h-8 w-8 place-items-center rounded-lg bg-[#6366F1] font-semibold">B</span>
        <span className="text-lg font-semibold tracking-tight">Brightloop</span>
      </div>
      <div className="flex-1 space-y-6 overflow-y-auto px-3 py-4">
        {groups.map((g) => (
          <div key={g.title}>
            <p className="px-2 pb-1 text-xs font-medium text-slate-500">{g.title}</p>
            <ul className="space-y-0.5">
              {g.items.map((item) => {
                const Icon = (Icons as unknown as Record<string, Icons.LucideIcon>)[item.icon] ?? Icons.Circle;
                const active = pathname === item.href || pathname.startsWith(item.href + "/");
                return (
                  <li key={item.href}>
                    <Link
                      href={item.href}
                      aria-current={active ? "page" : undefined}
                      className={`flex items-center gap-3 rounded-lg px-3 py-2 text-sm outline-none transition-colors focus-visible:ring-2 focus-visible:ring-[#6366F1] ${
                        active ? "bg-[#6366F1] text-white" : "hover:bg-slate-700/60 hover:text-white"
                      }`}
                    >
                      <Icon className="h-4 w-4 shrink-0" aria-hidden />
                      {item.label}
                    </Link>
                  </li>
                );
              })}
            </ul>
          </div>
        ))}
      </div>
      <div className="border-t border-slate-700 px-5 py-4 text-sm">
        <p className="truncate font-medium text-white">{userName}</p>
        <p className="text-xs text-slate-400">{role.charAt(0) + role.slice(1).toLowerCase()}</p>
      </div>
    </nav>
  );

  return (
    <>
      {/* Mobile top bar + drawer */}
      <div className="sticky top-0 z-30 flex h-14 items-center justify-between bg-[#1E293B] px-4 text-white lg:hidden">
        <span className="font-semibold">Brightloop</span>
        <button onClick={() => setOpen((o) => !o)} aria-expanded={open} aria-label="Toggle navigation" className="rounded-md p-2 focus-visible:ring-2 focus-visible:ring-[#6366F1]">
          {open ? <Icons.X className="h-5 w-5" /> : <Icons.Menu className="h-5 w-5" />}
        </button>
      </div>
      {open && <div className="fixed inset-0 z-40 bg-slate-900/50 lg:hidden" onClick={() => setOpen(false)} aria-hidden />}
      <aside className={`fixed inset-y-0 left-0 z-50 w-64 transform transition-transform lg:static lg:z-auto lg:translate-x-0 ${open ? "translate-x-0" : "-translate-x-full"}`}>
        {content}
      </aside>
    </>
  );
}
