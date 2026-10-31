import { runBillingCycle } from "../src/lib/billing/processor";

runBillingCycle()
  .then((r) => {
    console.log(JSON.stringify(r, null, 2));
    process.exit(r.failures.length ? 1 : 0);
  })
  .catch((e) => {
    console.error(e);
    process.exit(1);
  });
