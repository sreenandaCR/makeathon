
function viewInvoice(invoiceId) {

    let invoiceDetails = {

        INV001: {
            plan: "Basic Plan",
            subtotal: 299,
            gst: 53.82,
            total: 352.82,
            status: "Paid"
        },

        INV002: {
            plan: "Standard Plan",
            subtotal: 599,
            gst: 107.82,
            total: 706.82,
            status: "Pending"
        },

        INV003: {
            plan: "Premium Plan",
            subtotal: 999,
            gst: 179.82,
            total: 1178.82,
            status: "Overdue"
        }

    };

    let invoice = invoiceDetails[invoiceId];

    if (invoice) {

        alert(
            "INVOICE: " + invoiceId +
            "\nPlan: " + invoice.plan +
            "\nSubtotal: ₹" + invoice.subtotal +
            "\nGST: ₹" + invoice.gst +
            "\nTotal: ₹" + invoice.total +
            "\nStatus: " + invoice.status
        );

    }

}
