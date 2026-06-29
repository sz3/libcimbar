// simple test scaffolding to run qunit in browser
// using puppeteer for now, playwright might be better???
// copy+paste, ai cross reference, I didn't write this,
// run at your own risk etc (it's pretty simple though)
const puppeteer = require('puppeteer');
const path = require('path');
const QUnit = require('qunit');

const testPages = ['test_recv.html', 'test_send.html'];

async function runTestPage(browser, testPagePath) {
  const page = await browser.newPage();
  page.setDefaultNavigationTimeout(30000);
  page.setDefaultTimeout(30000);

  page.on('console', msg => console.log(`[${testPagePath}] BROWSER CONSOLE:`, msg.text()));
  page.on('pageerror', err => console.error(`[${testPagePath}] PAGE ERROR:`, err));

  let resolveStats;
  const statsPromise = new Promise(resolve => { resolveStats = resolve; });

  await page.exposeFunction('onLogMe', (details) => {
    if (!details.result) {
      details.expected = JSON.stringify(details.expected);
      details.actual = JSON.stringify(details.actual);
      console.error(details);
    }
  });

  await page.exposeFunction('onQUnitDone', (stats) => {
    resolveStats(stats);
  });

  await page.goto(`http://localhost:8080/${testPagePath}`);

  // Hook QUnit log and done handlers inside the page
  await page.evaluate(() => {
    return new Promise(resolve => {
      QUnit.log(details => {
        window.onLogMe(details);
      });

      QUnit.done(stats => {
        window.onQUnitDone(stats);
        resolve();
      });
    });
  });

  const stats = await statsPromise;
  await page.close();
  return stats;
}

(async () => {
  const browser = await puppeteer.launch({ headless: true, args: ['--no-sandbox', '--disable-setuid-sandbox'] });

  let totalFailed = 0;
  for (const testPagePath of testPages) {
    console.log(`Running tests: ${testPagePath}`);
    const stats = await runTestPage(browser, testPagePath);
    if (stats.failed > 0) {
      console.error(`FAILED [${testPagePath}]: ${stats.failed} assertions failed.`);
      totalFailed += stats.failed;
    } else {
      console.log(`PASSED [${testPagePath}]`);
    }
  }

  await browser.close();

  if (totalFailed > 0) {
    console.error(`Total QUnit failures: ${totalFailed}`);
    process.exit(totalFailed);
  } else {
    console.log('All QUnit Tests Passed!');
    process.exit(0);
  }
})();
