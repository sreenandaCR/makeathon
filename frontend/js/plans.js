
function changePrices() {

    let cycle = document.getElementById("billingCycle").value;

    let prices = document.querySelectorAll(".price");

    prices.forEach(function(price) {

        if (cycle === "monthly") {
            price.textContent = "₹" + price.dataset.monthly + "/month";
        }
        else {
            price.textContent = "₹" + price.dataset.yearly + "/year";
        }

    });
}


function selectPlan(planName) {

    let message = document.getElementById("planMessage");

    message.textContent =
        "You selected " + planName +
        ". Subscription request will be connected to the backend later.";

}
