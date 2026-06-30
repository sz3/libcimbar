var Report = function () {

  return {
    prevent_sleep: function () {
      setTimeout(Main.prevent_sleep, 0);
    },

    setAspectRatio: function (idealRatio) {
      Main.setAspectRatio(idealRatio);
    },

    setActive: function (active) {
      Main.setActive(active);
    },

    setHTML: function (id, msg) {
      Main.setHTML(id, msg);
    },

    setTitle: function (msg) {
      Main.setTitle(msg);
    },

    startWasm: function (result) {
      Main.on_ww_init(!result);
    },

    handleWorkerMessage: function (event) {
      if (event.error) {
        console.error('[report] error ' + event.message);
        return;
      }
      const { fun, args } = event.data;
      //console.log(event.data);
      if (typeof Report[fun] !== 'function') {
        console.warn('[report] Unknown worker message type: ' + fun);
        return;
      }
      Report[fun](...args);
    }
  };
}();


var Main = function () {

  // configurable
  var _interval = 66;
  var _colorBalance = false;

  // internal
  var _ww = undefined;

  var _showStats = false;
  var _wakeLock = undefined;

  // cached
  var _idealRatio = 1;

  function toggleFullscreen() {
    if (document.fullscreenElement) {
      return document.exitFullscreen();
    }
    else {
      return document.documentElement.requestFullscreen();
    }
  }

  // public interface
  return {
    init: function (canvas) {
      if (!_ww) { // no webworker, use main thread
        console.log("main thread impl");
        Send.init_window(canvas);
        Main.setMode('B');
        Send.nextFrame();
        return;
      }
    },

    init_ww: function (canvas) {
      const urlParams = new URLSearchParams(window.location.search);
      const use_ww = urlParams.get('ww') || window.location.hash == "#ww=1";
      if (!use_ww || typeof OffscreenCanvas === 'undefined' || typeof Worker === 'undefined') {
        return false;
      }
      console.log("ww impl");
      var offscreen = canvas.transferControlToOffscreen();
      _ww = new Worker('send-worker.js');
      _ww.onmessage = Report.handleWorkerMessage;
      _ww.onerror = function (err) {
        console.error('[send] Worker error: ', err);
      };
      _ww.postMessage({ fun: 'init_window', args: [offscreen] }, [offscreen]);
      return true;
    },

    on_ww_init: function (force_local) {
      console.log('on ww init ' + force_local);

      if (force_local) {
        _ww = undefined;
        // force refresh in main thread mode
        const cleanURL = window.location.origin + window.location.pathname;
        window.location.replace(cleanURL);
        return;
      }

      Main.setMode('B');
      _ww.postMessage({ fun: 'nextFrame', args: [] });
    },


    check_GL_enabled: function () {
      var testCanvas = document.createElement('canvas');
      if (!testCanvas.getContext("webgl2") && !testCanvas.getContext("webgl")) {
        var elem = document.getElementById('dragdrop');
        elem.classList.add("error");
      }
    },

    resize: function () {
      // reset zoom
      var canvas = document.getElementById('canvas');
      var width = window.innerWidth - 10;
      var height = window.innerHeight - 10;
      Main.scaleCanvas(canvas, width, height);
      Main.alignInvisibleClick(canvas);
      Main.checkNavButtonOverlap();
    },

    toggleFullscreen: function () {
      toggleFullscreen().then(Main.resize);
      Main.togglePause(true);
    },

    togglePause: function (pause) {
      if (_ww) {
        _ww.postMessage({ fun: 'togglePause', args: [pause] });
      }
      else {
        Send.togglePause(pause);
      }
    },

    scaleCanvas: function (canvas, width, height) {
      // using ratio from current config,
      // determine optimal dimensions and rotation
      var needRotate = _idealRatio > 1 && height > width;
      if (_ww) {
        _ww.postMessage({ fun: 'rotate_window', args: [needRotate] });
      }
      else {
        Send.rotate_window(needRotate);
      }

      var ourRatio = needRotate ? height / width : width / height;

      var xdim = needRotate ? height : width;
      var ydim = needRotate ? width : height;
      if (ourRatio > _idealRatio) {
        xdim = Math.floor(xdim * _idealRatio / ourRatio);
      }
      else if (ourRatio < _idealRatio) {
        ydim = Math.floor(ydim * ourRatio / _idealRatio);
      }

      console.log(xdim + "x" + ydim);
      if (needRotate) {
        canvas.style.width = ydim + "px";
        canvas.style.height = xdim + "px";
      }
      else {
        canvas.style.width = xdim + "px";
        canvas.style.height = ydim + "px";
      }
    },

    alignInvisibleClick: function (canvas) {
      canvas = canvas || document.getElementById('canvas');
      var cpos = canvas.getBoundingClientRect();
      var invisible_click = document.getElementById("invisible_click");
      invisible_click.style.width = canvas.style.width;
      invisible_click.style.height = canvas.style.height;
      invisible_click.style.top = cpos.top + "px";
      invisible_click.style.left = cpos.left + "px";
      invisible_click.style.zoom = canvas.style.zoom;
    },

    prevent_sleep: async function () {
      if (_wakeLock) {
        return;
      }
      const requestWakeLock = async () => {
        try {
          _wakeLock = await navigator.wakeLock.request('screen');
          console.log('got wake lock!');
          _wakeLock.addEventListener('release', () => {
            _wakeLock = undefined;
          });
        } catch (err) { }
      };
      requestWakeLock();
    },

    importFile: function (fblob) {
      if (!_ww) {
        Send.importFile(fblob);
        return;
      }

      _ww.postMessage({ fun: 'importFile', args: [fblob] });
    },

    dragDrop: function (event) {
      console.log("drag drop?");
      console.log(event);
      const files = event.dataTransfer.files;
      if (files && files.length === 1) {
        Main.importFile(files[0]);
      }
    },

    checkNavButtonOverlap: function () {
      var nav = document.getElementById("nav-button");
      var navBounds = nav.getBoundingClientRect();
      var canvas = document.getElementById('canvas').getBoundingClientRect();
      if (navBounds.right > canvas.left && navBounds.bottom > canvas.top) {
        nav.classList.add("hide");
      }
      else {
        nav.classList.remove("hide");
      }
    },

    clickNav: function () {
      document.getElementById("nav-button").focus();
    },

    blurNav: function (pause) {
      if (pause === undefined) {
        pause = true;
      }
      document.getElementById("nav-button").blur();
      document.getElementById("nav-content").blur();
      document.getElementById("nav-find-file-link").blur();
      Main.togglePause(pause);
    },

    clickFileInput: function () {
      document.getElementById("file_input").click();
    },

    fileInput: function (ev) {
      console.log("file input: " + ev);
      var file = document.getElementById('file_input').files[0];
      if (file) {
        Main.importFile(file);
      }
      Main.blurNav(false);
    },

    setAspectRatio: function (idealRatio) {
      _idealRatio = idealRatio;
      Main.resize();
    },

    setActive: function (active) {
      // hide cursor when there's a barcode active
      var invisi = document.getElementById("invisible_click");
      invisi.classList.remove("active");
      invisi.classList.add("active");
    },

    setMode: function (mode_str) {
      let modeVal = 68;
      if (mode_str == "4C") {
        modeVal = 4;
      }
      else if (mode_str == "Bu") {
        modeVal = 66;
      }
      else if (mode_str == "Bm") {
        modeVal = 67;
      }

      // will trigger setAspectRatio callback
      if (_ww) {
        _ww.postMessage({ fun: 'setMode', args: [modeVal] });
      }
      else {
        Send.setMode(modeVal);
      }

      var nav = document.getElementById("nav-container");
      if (modeVal == 4) {
        nav.classList.remove("mode-b");
        nav.classList.add("mode-4c");
        nav.classList.remove("mode-b");
        nav.classList.remove("mode-bm");
        nav.classList.remove("mode-bu");
      } else if (modeVal == 66) {
        nav.classList.add("mode-bu");
        nav.classList.remove("mode-b");
        nav.classList.remove("mode-bm");
        nav.classList.remove("mode-4c");
      } else if (modeVal == 67) {
        nav.classList.add("mode-bm");
        nav.classList.remove("mode-b");
        nav.classList.remove("mode-bu");
        nav.classList.remove("mode-4c");
      } else if (modeVal == 68) {
        nav.classList.add("mode-b");
        nav.classList.remove("mode-bm");
        nav.classList.remove("mode-bu");
        nav.classList.remove("mode-4c");
      } else {
        nav.classList.remove("mode-b");
        nav.classList.remove("mode-bm");
        nav.classList.remove("mode-bu");
        nav.classList.remove("mode-4c");
      }
    },

    setFPS: function (val) {
      if (!val) {
        return;
      }
      if (_ww) {
        _ww.postMessage({ fun: 'setFPS', args: [val] });
      }
      else {
        Send.setFPS(val);
      }
    },

    setHTML: function (id, msg) {
      document.getElementById(id).innerHTML = msg;
    },

    setTitle: function (msg) {
      document.title = "Cimbar: " + msg;
    }
  };
}();

window.addEventListener('keydown', function (e) {
  e = e || event;
  if (e.target instanceof HTMLBodyElement) {
    if (e.key == 'Enter' || e.keyCode == 13 ||
      e.key == 'Tab' || e.keyCode == 9 ||
      e.key == 'Space' || e.keyCode == 32
    ) {
      Main.clickNav();
      e.preventDefault();
    }
    else if (e.key == 'Backspace' || e.keyCode == 8) {
      Main.togglePause(true);
      e.preventDefault();
    }
  }
  else {
    if (e.key == 'Escape' || e.keyCode == 27 ||
      e.key == 'Backspace' || e.keyCode == 8 ||
      e.key == 'End' || e.keyCode == 35 ||
      e.key == 'Home' || e.keyCode == 36
    ) {
      Main.blurNav();
    }
    else if (e.key == 'Tab' || e.keyCode == 9 ||
      e.key == 'ArrowDown' || e.keyCode == 40
    ) {
      var nav = document.getElementById('nav-button');
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      if (nav.classList.contains('attention')) {
        nav.classList.remove('attention');
        links[0].classList.add('attention');
        return;
      }
      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          var next = i + 1 == links.length ? nav : links[i + 1];
          links[i].classList.remove('attention');
          next.classList.add('attention');
          break;
        }
      }
    }
    else if (e.key == 'ArrowUp' || e.keyCode == 38) {
      var nav = document.getElementById('nav-button');
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      if (nav.classList.contains('attention')) {
        nav.classList.remove('attention');
        links[links.length - 1].classList.add('attention');
        return;
      }

      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          var next = i == 0 ? nav : links[i - 1];
          links[i].classList.remove('attention');
          next.classList.add('attention');
          break;
        }
      }
    }
    else if (e.key == 'Enter' || e.keyCode == 13 ||
      e.key == ' ' || e.keyCode == 32
    ) {
      var nav = document.getElementById('nav-button');
      if (nav.classList.contains('attention')) {
        Main.blurNav();
        return;
      }
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          links[i].click();
        }
      }
    }
  }
}, true);

window.addEventListener("touchstart", function (e) {
  e = e || event;
  Main.togglePause(true);
}, false);

window.addEventListener("touchend", function (e) {
  e = e || event;
  Main.togglePause(false);
}, false);

window.addEventListener("touchcancel", function (e) {
  e = e || event;
  Main.togglePause(false);
}, false);

window.addEventListener("dragover", function (e) {
  e = e || event;
  e.preventDefault();

  document.body.style["opacity"] = 0.5;
}, false);

window.addEventListener("dragleave", function (e) {
  e = e || event;
  e.preventDefault();

  document.body.style["opacity"] = 1.0;
}, false);

window.addEventListener("drop", function (e) {
  e = e || event;
  e.preventDefault();
  e.stopPropagation();
  Main.dragDrop(e);
  document.body.style["opacity"] = 1.0;
}, false);

window.addEventListener('resize', () => {
  Main.resize();
});
