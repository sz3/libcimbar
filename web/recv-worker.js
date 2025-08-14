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

      try {
        // then decode in wasm, fool
        const fountainBuff = RecvWorker.fountainBuff();
        var len = Module._cimbard_scan_extract_decode(RecvWorker.imgBuff().byteOffset, width, height, type, fountainBuff.byteOffset, fountainBuff.length);

        if (len <= 0) {
          var errmsg = RecvWorker.get_error();
          errmsg = len + " " + errmsg;
          //console.log(errmsg);
          if (len == 0) {
            self.postMessage({ nodata: true, res: errmsg });
          }
          else if (len != -3) {
            self.postMessage({ error: true, res: errmsg });
          }
        }
        else { //if (len > 0) {
          console.log('len is ' + len);
          //self.postMessage({ error: true, res: len });
          const msgbuf = new Uint8Array(Module.HEAPU8.buffer, fountainBuff.byteOffset, len).slice();
          console.log(msgbuf);
          self.postMessage(msgbuf.buffer, [msgbuf.buffer]);
        }
        // in main, const receivedArray = new Uint8Array(event.data);
      } catch (e) {
        console.log(e);
      }

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

  if (event.data.config) {
    console.log('attempting config');
    try {
      Module._cimbard_configure_decode(event.data.color_bits, event.data.mode_val);
    } catch (e) {
      self.postMessage({ type: 'config', error: e });
    }
    return;
  }

  // assert 'vf' in data?
  RecvWorker.on_frame(event.data)
};
