
function acceptQuotation(id) {

    let confirmAction = confirm(
        "Are you sure you want to accept " + id + "?"
    );

    if (confirmAction) {

        document.getElementById("quotationMessage").textContent =
            id + " acceptance request submitted successfully.";

    }

}

function rejectQuotation(id) {

    let confirmAction = confirm(
        "Are you sure you want to reject " + id + "?"
    );

    if (confirmAction) {

        document.getElementById("quotationMessage").textContent =
            id + " rejection request submitted successfully.";

    }

}
