import type { Role } from "@/lib/enums";

export interface NavItem {
  label: string;
  href: string;
  roles: Role[];
  icon: string; // lucide-react icon name
}
export interface NavGroup {
  title: string;
  items: NavItem[];
}

export const NAV: NavGroup[] = [
  {
    title: "Overview",
    items: [{ label: "Finance dashboard", href: "/dashboard", roles: ["ADMIN", "STAFF"], icon: "LayoutDashboard" }],
  },
  {
    title: "Sales",
    items: [
      { label: "Subscriptions", href: "/subscriptions", roles: ["ADMIN", "STAFF"], icon: "Repeat" },
      { label: "Quotation templates", href: "/quotation-templates", roles: ["ADMIN", "STAFF"], icon: "FileText" },
    ],
  },
  {
    title: "Catalogue",
    items: [
      { label: "Products", href: "/products", roles: ["ADMIN", "STAFF"], icon: "Package" },
      { label: "Plans", href: "/plans", roles: ["ADMIN", "STAFF"], icon: "Layers" },
      { label: "Discounts", href: "/discounts", roles: ["ADMIN"], icon: "Percent" },
    ],
  },
  {
    title: "Billing",
    items: [
      { label: "Invoices", href: "/invoices", roles: ["ADMIN", "STAFF"], icon: "Receipt" },
      { label: "Payments", href: "/payments", roles: ["ADMIN", "STAFF"], icon: "Wallet" },
    ],
  },
  {
    title: "My account",
    items: [
      { label: "My subscriptions", href: "/subscriptions", roles: ["CUSTOMER"], icon: "Repeat" },
      { label: "My invoices", href: "/invoices", roles: ["CUSTOMER"], icon: "Receipt" },
      { label: "Payment history", href: "/payments", roles: ["CUSTOMER"], icon: "Wallet" },
    ],
  },
  {
    title: "Administration",
    items: [
      { label: "Staff", href: "/admin/staff", roles: ["ADMIN"], icon: "Users" },
      { label: "Tax rules", href: "/admin/taxes", roles: ["ADMIN"], icon: "Landmark" },
    ],
  },
];

/** Role filtering happens on the server so hidden links never reach a customer's browser. */
export function navFor(role: Role): NavGroup[] {
  return NAV.map((g) => ({ ...g, items: g.items.filter((i) => i.roles.includes(role)) })).filter((g) => g.items.length > 0);
}
