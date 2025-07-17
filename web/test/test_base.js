QUnit.module( "base" );
QUnit.config.reorder = false;

QUnit.testStart(async function(details) {
  const img = document.getElementById('example_frame');
  const imageBitmap = await window.createImageBitmap(img);
  imageBitmap.requestVideoFrameCallback = function() {};
  
  try {
    Recv.init_video(imageBitmap);
  } catch (ex) {}

  await WAIT_UNTIL_READY;
  await Recv.ww_ready;
});

QUnit.testDone(function(details) {

});

QUnit.test( "simple", async function( assert ) {

  console.log("IT'S GO TIME")
  
  Recv.on_frame(0,'');
  assert.ok( true);
  assert.equal( 1, 2 );
});
