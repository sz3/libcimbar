QUnit.module("base");
QUnit.config.reorder = false;

function wait_for(assert, block) {
  var done = assert.async();
  return new Promise(resolve => {
    // A function that checks the condition.
    const check = () => {
      // If the condition is met, resolve the promise.
      var res;
      try {
        res = block();
      } catch (ex) {
        assert.ok(false, ex);
      }
      if (res) {
        done();
        resolve(res);
      } else {
        // schedule the next check.
        requestAnimationFrame(check);
      }
    };

    check();
  });
}


QUnit.testStart(async function (details) {
  const img = document.getElementById('example_frame');
  const imageBitmap = await window.createImageBitmap(img);
  imageBitmap.requestVideoFrameCallback = function () { };

  try {
    Recv.init_video(imageBitmap);
  } catch (ex) { }

  await WAIT_UNTIL_READY;
  await Recv.ww_ready;
});

QUnit.testDone(function (details) {

});

QUnit.test("stable decode", async function (assert) {

  Recv.on_frame(0, '');

  const progress_container = document.getElementById('progress_bars');
  const query = '#progress_bars > div[class="progress"]';

  const progress = await wait_for(assert, () => {
    return document.querySelector(query);
  });


  assert.ok(progress);
  assert.equal(progress.style.width, "30.7692%");
});
