QUnit.module("base");
QUnit.config.reorder = false;

let _zstdCalls = [];

Zstd.download_blob = function (name, blob) {
  console.log(blob);
  _zstdCalls.push({ download_blob: [name, blob.size] });
};

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

async function load_image(num) {
  const img = document.getElementById('example_frame' + num);
  const imageBitmap = await window.createImageBitmap(img);
  imageBitmap.requestVideoFrameCallback = function () { };

  try {
    Recv.init_video(imageBitmap);
  } catch (ex) { }
}

QUnit.testStart(async function (details) {
  await WAIT_UNTIL_READY;
  await Recv.ww_ready;
});

QUnit.testDone(function (details) {

});

QUnit.test("stable decode", async function (assert) {
  await load_image(0);
  Recv.on_frame(0, '');

  const progress_container = document.getElementById('progress_bars');
  const query = '#progress_bars > div[class="progress"]';

  const pro0 = await wait_for(assert, () => {
    return document.querySelector(query);
  });
  assert.equal(pro0.style.width, "30.7692%");

  pro0.remove();

  await load_image(1);
  Recv.on_frame(0, '');

  const pro1 = await wait_for(assert, () => {
    return document.querySelector(query);
  });
  assert.equal(pro1.style.width, "61.5385%");

  pro1.remove();

  await load_image(2);
  Recv.on_frame(0, '');

  const pro2 = await wait_for(assert, () => {
    return document.querySelector(query);
  });
  assert.equal(pro2.style.width, "92.3077%");
  assert.deepEqual(_zstdCalls, []);

  pro2.remove();

  // last one
  await load_image(3);
  Recv.on_frame(0, '');

  const pro3 = await wait_for(assert, () => {
    return document.querySelector(query);
  });
  assert.equal(pro3.style.width, "100%");

  assert.deepEqual(_zstdCalls, [
    { download_blob: ["576454656.23586", 23947] }
  ]);
});
