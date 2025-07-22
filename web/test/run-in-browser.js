// simple test scaffolding to run qunit in browser
// using puppeteer for now, playwright might be better???
// copy+paste, ai cross reference, I didn't write this,
// run at your own risk etc (it's pretty simple though)
const puppeteer = require('puppeteer');
const path = require('path');
const QUnit = require('qunit');

(async () => {
  const browser = await puppeteer.launch({ headless: true, args: ['--no-sandbox', '--disable-setuid-sandbox'] });
  const page = await browser.newPage();

  page.on('console', msg => console.log('BROWSER CONSOLE:', msg.text()));
  page.on('pageerror', err => console.error('PAGE ERROR:', err));

  await page.exposeFunction('onLogMe', (details) => {
    if (!details.result) {
      details.expected = JSON.stringify(details.expected);
      details.actual = JSON.stringify(details.actual);
      console.error(details);
    }
  });

  // Expose a function to capture QUnit results
  await page.exposeFunction('onQUnitDone', (stats) => {
    if (stats.failed > 0) {
      console.error(stats);
      console.error(`QUnit Tests Failed: ${stats.failed} assertions failed.`);
      process.exit(stats.failed); // nonzero is failure
    } else {
      console.log('All QUnit Tests Passed!');
      process.exit(0);
    }
  });

  // Navigate to your QUnit test page
  const testPagePath = '/test_recv.html';
  await page.goto(`http://localhost:8080/${testPagePath}`);

  // Wait for QUnit to finish
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

  await browser.close();
})();
