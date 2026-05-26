QUnit.module("send");
QUnit.config.reorder = false;
QUnit.config.testTimeout = 10000;

QUnit.testStart(async function () {
  await WAIT_UNTIL_READY;
});

QUnit.test("encode+render", async function (assert) {
  // put some data on the heap
  const testData = new TextEncoder().encode(
    "testing test testing test, unfortunately this will compress well and probably isn't the best test string\n".repeat(54)
  );
  const fakefile = new File([testData], "testfile.bin", { type: "application/octet-stream" });
  Main.importFile(fakefile);

  // wait for render
  await new Promise(resolve => setTimeout(resolve, 500));

  const canvas = document.getElementById('canvas');
  assert.ok(
    canvas.width > 0 && canvas.height > 0,
    "canvas not very big is it (" + canvas.width + "x" + canvas.height + ")"
  );

  // get the WebGL context
  const gl = canvas.getContext('webgl2');
  assert.ok(gl, "WebGL context is available");
  if (!gl) {
    return;
  }

  // sample 64x64 pixels from near the bottom left
  const sw = Math.min(canvas.width, 64);
  const sh = Math.min(canvas.height, 64);
  const pixels = new Uint8Array(4 * sw * sh);
  // we read from inside the anchors for hopefully obvious reasons
  gl.readPixels(60, 60, sw, sh, gl.RGBA, gl.UNSIGNED_BYTE, pixels);

  console.log("got some pixels?");
  console.log(pixels);

  // count pixels with rgb set
  let coloredPixels = 0;
  for (let i = 0; i < pixels.length; i += 4) {
    if (pixels[i] > 0 || pixels[i + 1] > 0 || pixels[i + 2] > 0) {
      coloredPixels++;
    }
  }

  const totalPixels = sw * sh;
  assert.ok(
    coloredPixels > 10,
    "canvas has " + coloredPixels + "/" + totalPixels +
    " non-black pixels after encoding"
  );
});
