import Link from "next/link";
import { logoutAction } from "@/actions/auth";

export interface Crumb {
  label: string;
  href?: string;
}

export function Topbar({ crumbs, search }: { crumbs: Crumb[]; search?: { action: string; placeholder: string; defaultValue?: string } }) {
  return (
    <header className="flex flex-wrap items-center justify-between gap-3 border-b border-slate-200 bg-white px-4 py-3 sm:px-6">
      <nav aria-label="Breadcrumb">
        <ol className="flex items-center gap-1.5 text-sm text-slate-500">
          {crumbs.map((c, i) => (
            <li key={c.label} className="flex items-center gap-1.5">
              {i > 0 && <span aria-hidden>/</span>}
              {c.href && i < crumbs.length - 1 ? (
                <Link href={c.href} className="hover:text-[#6366F1]">{c.label}</Link>
              ) : (
                <span aria-current="page" className="font-medium text-slate-900">{c.label}</span>
              )}
            </li>
          ))}
        </ol>
      </nav>
      <div className="flex items-center gap-3">
        {search && (
          <form action={search.action} role="search">
            <input
              type="search"
              name="q"
              defaultValue={search.defaultValue}
              placeholder={search.placeholder}
              className="w-44 rounded-lg border border-slate-300 bg-slate-50 px-3 py-1.5 text-sm outline-none focus:border-[#6366F1] focus:ring-2 focus:ring-[#6366F1]/30 sm:w-64"
            />
          </form>
        )}
        <form action={logoutAction}>
          <button className="rounded-lg px-3 py-1.5 text-sm text-slate-600 hover:bg-slate-100">Sign out</button>
        </form>
      </div>
    </header>
  );
}
