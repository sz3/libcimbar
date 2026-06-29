'use strict';

// polyfill
if (typeof window === 'undefined') {
  self.window = self;
}

// stubbing extra things out for emscripten's GLFW
if (typeof window.matchMedia === 'undefined') {
  window.matchMedia = function () {
    return {
      matches: false,
      addEventListener: function () { },
      removeEventListener: function () { },
      addListener: function () { },        // legacy Emscripten / Safari compat
      removeListener: function () { },
    };
  };
}
if (typeof document === 'undefined') {
  self.document = {
    fullscreenElement: null,
    mozFullScreenElement: null,
    webkitFullscreenElement: null,
    msFullscreenElement: null,
    getElementById: function () { return null; },
    addEventListener: function () { },
    removeEventListener: function () { },
    visibilityState: 'visible',
    hidden: false,
  };
}

let _wasmInitialized = false;

var Module = {
  preRun: [],
  onRuntimeInitialized: function load_done_callback() {
    console.info("The Module is loaded and is accessible here", Module);
    _wasmInitialized = true;
    try {
      Send.init_window(Module.canvas);
      self.postMessage({ fun: 'startWasm', args: [true] });
    } catch (ex) {
      console.log("unexpected init error: " + ex);
      self.postMessage({ fun: 'startWasm', args: [false] });
    }

  }
};

var Report = function () {

  // might be able to make this more magical...
  return {
    prevent_sleep: function () {
      self.postMessage({ "fun": "prevent_sleep", "args": [] });
    },

    setAspectRatio: function (idealRatio) {
      self.postMessage({ "fun": "setAspectRatio", "args": [idealRatio] });
    },

    setActive: function (active) {
      self.postMessage({ "fun": "setActive", "args": [active] });
    },

    setHTML: function (id, msg) {
      self.postMessage({ "fun": "setHTML", "args": [id, msg] });
    },

    setTitle: function (msg) {
      self.postMessage({ "fun": "setTitle", "args": [msg] });
    },

    startWasm: function (result) {
      self.postMessage({ "fun": "startWasm", "args": [result] });
    }
  };
}();

importScripts('send.js');

self.onmessage = async (event) => {

  const { fun, args } = event.data;
  if (typeof Send[fun] !== 'function') {
    console.warn('[send-worker] Unknown message type: ' + fun);
    return;
  }

  console.log('got a message ' + fun);

  if (fun === 'init_window') {
    Module.canvas = args[0];
    importScripts('cimbar_js.js');
    return;
  }

  if (!_wasmInitialized) {
    console.log('we got no wasm yet :(');
    self.postMessage({ fun: 'startWasm', args: [false] });
    return;
  }

  try {
    Send[fun](...args);
  } catch (ex) {
    console.log("unexpected error: " + ex);
    self.postMessage({ error: true, message: ex });
  }
};
