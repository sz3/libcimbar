let _wasmInitialized = false;
let _buffs = {};

var Module = {
  preRun: [],
  onRuntimeInitialized: function load_done_callback() {
    console.info("The Module is loaded and is accessible here", Module);
    _wasmInitialized = true;
    self.postMessage({ type: 'startWasm', ready: "ready!" });
  }
};

var RecvWorker = function () {

  // public interface
  return {
    on_frame: function (data) {
      const pixels = data.pixels;
      const format = data.format;
      const width = data.width;
      const height = data.height;
      const mode = data.mode;
      if (mode) {
        Module._cimbard_configure_decode(mode);
      }

      try {
        //console.log(vf);
        // malloc iff necessary
        RecvWorker.mallocAll(pixels.length);
        const imgBuff = RecvWorker.imgBuff();
        imgBuff.set(pixels, 0); // copy
      } catch (e) {
        console.log(e);
      }

      var type = 4;
      if (format == "NV12") {
        type = 12;
      }
      else if (format == "I420") {
        type = 420;
      }

      // then decode in wasm, fool
      const fountainBuff = RecvWorker.fountainBuff();
      var len = Module._cimbard_scan_extract_decode(RecvWorker.imgBuff().byteOffset, width, height, type, fountainBuff.byteOffset, fountainBuff.length);
      if (len <= 0) {
        var errmsg = RecvWorker.get_error();
        errmsg = len + " " + errmsg;
        let msg = { res: errmsg };
        if (len == 0)
          msg.nodata = true;
        else if (len == -3)
          msg.failed_extract = true;
        else
          msg.error = true;
        self.postMessage(msg);
      }
      else { //if (len > 0) {
        console.log('len is ' + len);
        const msgbuf = new Uint8Array(Module.HEAPU8.buffer, fountainBuff.byteOffset, len).slice();
        //console.log(msgbuf);
        self.postMessage({ mode: mode, buff: msgbuf }, [msgbuf.buffer]);
      }
      // in main, const receivedArray = event.data.buff;
    },

    get_error: function () {
      const errbuff = RecvWorker.mallocPlease("error", 256);
      const errlen = Module._cimbard_get_report(errbuff.byteOffset, errbuff.length);
      if (errlen > 0) {
        const errview = new Uint8Array(Module.HEAPU8.buffer, errbuff.byteOffset, errlen);
        const decoder = new TextDecoder();
        return decoder.decode(errview);
      }
      return "";
    },

    mallocPlease: function (name, size) {
      if (_buffs[name] === undefined || size > _buffs[name].length) {
        try {
          if (size > _buffs[name].length) {
            console.log("resizing " + name + " buff from " + _buffs[name].length + " to " + size);
            Module._free(_buffs[name].byteOffset);
          }
        } catch (e) { } // if we're leaking memory we'll find out the hard way 
        const dataPtr = Module._malloc(size);
        _buffs[name] = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
      }
      return _buffs[name];
    },

    mallocAll: function (imgsize) {
      RecvWorker.mallocPlease("img", imgsize);

      const bufsize = Module._cimbard_get_bufsize();
      RecvWorker.mallocPlease("fountain", bufsize);
    },

    imgBuff: function () {
      return _buffs['img'];
    },

    fountainBuff: function () {
      return _buffs['fountain'];
    }
  };
}();

importScripts('cimbar_js.js');

self.onmessage = async (event) => {

  if (!_wasmInitialized) {
    console.log('we got no wasm :(');
    self.postMessage({ type: 'startWasm', error: "no wasm" });
    return;
  }

  try {
    // assert 'vf' in data?
    RecvWorker.on_frame(event.data);
  } catch (ex) {
    console.log("unexpected error: " + ex);
    self.postMessage({ error: true, res: ex });
  }
};
