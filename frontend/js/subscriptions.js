
function requestRenewal(subscriptionId) {

    let message = document.getElementById("subscriptionMessage");

    message.textContent = "Renewal request for " + subscriptionId +
        " submitted for processing.";

}

function requestCancellation(subscriptionId) {

    let confirmAction = confirm(
        "Are you sure you want to request cancellation of " +
        subscriptionId + "?"
    );

    if (confirmAction) {

        let message = document.getElementById("subscriptionMessage");

        message.textContent = "Cancellation request for " +
            subscriptionId + " submitted for processing.";

    }

}
