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

  // Expose a function to capture QUnit results
  await page.exposeFunction('onQUnitDone', (failures) => {
    if (failures > 0) {
      console.error(`QUnit Tests Failed: ${failures} assertions failed.`);
      process.exit(failures); // nonzero is failure
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
      QUnit.done(function (details) {
        window.onQUnitDone(details.failed);
        resolve();
      });
    });
  });

  await browser.close();
})();
