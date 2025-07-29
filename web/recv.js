var Sink = function () {

  var _fountainBuff = undefined;
  var _errBuff = undefined;

  // public interface
  return {
    on_decode: function (buff) {
      if (_fountainBuff === undefined) {
        const size = Module._cimbard_get_bufsize(); // max length of buff. We could also resize as we go...
        const dataPtr = Module._malloc(size);
        _fountainBuff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
      }
      if (buff.length == 0) { // sanity check
        return;
      }
      _fountainBuff.set(buff);

      console.log('sink decode ' + _fountainBuff); //TODO: base64?
      var res = Module._cimbard_fountain_decode(_fountainBuff.byteOffset, buff.length);
      console.log("on decode got res " + res);

      const report = Sink.get_report();
      if (Array.isArray(report)) {
        Recv.render_progress(report);
      }
      else {
        Recv.set_HTML("tdec", "decode " + res + ". " + report);
      }

      if (res > 0) {
        const res32t = Number(res & 0xFFFFFFFFn);; // truncate BigInt res (int64_t) to a uint32_t
        Sink.reassemble_file(res32t);
      }
    },

    get_report: function () {
      const maxSize = 1024;
      if (_errBuff === undefined) {
        _errBuff = Module._malloc(maxSize);
      }
      const errlen = Module._cimbard_get_report(_errBuff, maxSize);
      if (errlen > 0) {
        const errview = new Uint8Array(Module.HEAPU8.buffer, _errBuff, errlen);
        const td = new TextDecoder();
        const text = td.decode(errview);
        try {
          return JSON.parse(text);
        } catch (error) {
          return text;
        }
      }
    },

    reassemble_file: function (id) {
      const size = Module._cimbard_get_filesize(id);
      //alert("we did it!?! " + size);
      const dataPtr = Module._malloc(size);
      const buff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
      try {
        var res = Module._cimbard_finish_copy(id, buff.byteOffset, buff.length);
        if (res < 0) {
          alert("reassemble_file failed :(" + res);
          console.log("we biffed it. :( " + res);
          Recv.set_HTML("errorbox", "reassemble_file failed :( " + res);
        }
        else {
          //Recv.download_bytes(buff, size + ".zst"); // size -> name, eventually
          Zstd.decompress(id + "." + size, buff);
        }
      } catch (error) {
        console.log("failed finish copy or download?? " + error);
      }
      // this needs to happen after decompress() completes
      // currently decompress is sync, so it's fine. But...
      Module._free(dataPtr);
      Recv.restart_video();
    }
  };
}();


var Recv = function () {

  var _counter = 0;
  var _renderTime = 0;
  var _captureNextFrame = 0;

  var _video = 0;
  var _workers = [];
  var _nextWorker = 0;
  var _workerReady;
  var _supportedFormats = ["NV12"]; // have cimbard_* return this somehow?

  function _toggleFullscreen() {
    if (document.fullscreenElement) {
      return document.exitFullscreen();
    }
    else {
      return document.documentElement.requestFullscreen();
    }
  }

  function _downloadHelper(name, bloburl) {
    var aaa = document.createElement('a');
    aaa.href = bloburl;
    aaa.download = name;
    document.body.appendChild(aaa);
    aaa.style = 'display: none';
    aaa.click();
    aaa.remove();
  }

  // public interface
  return {
    init: function (video, num_workers) {
      Recv.init_ww(num_workers);
      Recv.init_video(video);
    },

    set_error: function (msg) {
      Recv.set_HTML('errorbox', msg);
      return false;
    },

    ww_ready: new Promise(resolve => {
      _workerReady = resolve;
    }),

    init_ww: function (num_workers) {
      // clean up _workers if exists?
      _workers = [];
      for (let i = 0; i < num_workers; i++) {
        _workers.push(new Worker('recv-worker.js'));

        _workers[i].onmessage = (event) => {
          Recv.on_decode(i, event.data);
        };

        _workers[i].onerror = (error) => {
          console.error('Worker' + i + ' error:', error);
        };
      }
    },

    init_video: function (video) {
      _video = video;

      var constraints = {
        audio: false,
        video: {
          width: { min: 720, ideal: 1920 }, // Request HD but allow flexibility
          height: { min: 720, ideal: 1080 },
          aspectRatio: matchMedia('all and (orientation:landscape)').matches ? 16 / 9 : 9 / 16,
          facingMode: 'environment',
          exposureMode: 'continuous',
          focusMode: 'continuous',
          frameRate: { ideal: 15, max: 30 }, // we're not trying to set the user's phone on fire
        }
      };

      if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
        return Recv.set_error('mediaDevices not supported? :(');
      }

      navigator.mediaDevices.getUserMedia(constraints)
        .then(localMediaStream => {
          //console.log(localMediaStream);
          //console.dir(video);
          if ('srcObject' in video) {
            video.srcObject = localMediaStream;
          } else {
            video.src = URL.createObjectURL(localMediaStream); //deprecated
          }
          video.play();
          video.requestVideoFrameCallback(Recv.on_frame);
        })
        .catch(err => {
          console.error(`OH NO!!!!`, err);
        });
    },

    restart_video: function () {
      console.log("restart video?");
      if (_video) {
        return;
      }
      if (!('srcObject' in _video)) {
        console.log("can't restart video? :|")
        return;
      }
      // ios cancels the feed after we download. Restart it
      // check if localMediaStream.muted after a while???
      if (_video.srcObject.muted) {
        Recv.init_video(_video);
      }
    },

    download_bytes: function (buff, name) {
      var blob = new Blob([buff], { type: 'application/octet-stream' });
      var bloburl = window.URL.createObjectURL(blob);
      _downloadHelper(name, bloburl);
      setTimeout(function () {
        return window.URL.revokeObjectURL(bloburl);
      }, 1000);
    },

    on_decode: function (wid, data) {
      console.log('Main thread received message from worker' + wid + ':', data);
      if (data.res) {
        Recv.set_HTML("t" + wid, "avg red " + wid + " is " + data.res);
        return;
      }
      if (data.ready) {
        if (_workerReady)
          _workerReady();
        return;
      }

      const buff = new Uint8Array(data);
      Recv.set_HTML("t" + wid, "len() is " + buff.length + ", buff: " + buff);
      Sink.on_decode(buff);
    },

    on_frame: function (now, metadata) {
      //console.log("on frame");
      // https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame

      if (_workers.length == 0)
        return;
      if (_nextWorker >= _workers.length)
        _nextWorker = 0;

      var vf = undefined;
      try {
        vf = new VideoFrame(_video, { timestamp: now });
        const width = vf.displayWidth;
        const height = vf.displayHeight;
        Recv.set_HTML("errorbox", vf.format, true);

        // try to use the default format, but only if we can decode it...
        let vfparams = {};
        if (!_supportedFormats.includes(vf.format)) {
          vfparams.format = "RGBA";
        }
        const size = vf.allocationSize(vfparams);
        const buff = new Uint8Array(size);
        vf.copyTo(buff, vfparams);

        let format = vfparams.format || vf.format;
        if (format == "RGBA" && size != width * height * 4) {
          format = vf.format; //fallback
        }
        if (_captureNextFrame == 1) {
          _captureNextFrame = 0;
          Recv.download_bytes(buff, width + "x" + height + "x" + _counter + ".rgba");
        }
        _workers[_nextWorker].postMessage({ type: 'proc', pixels: buff, format: format, width: width, height: height }, [buff.buffer]);
      } catch (e) {
        console.log(e);
      }
      if (vf)
        vf.close();

      // schedule the next one
      _nextWorker += 1;
      _video.requestVideoFrameCallback(Recv.on_frame);
    },

    captureFrame: function () {
      _captureNextFrame = 1;
      alert("about to capture!");
    },

    download_bytes: function (buff, name) {
      var blob = new Blob([buff], { type: 'application/octet-stream' });
      Zstd.download_blob(name, blob);
    },

    render_progress: function (report) {
      console.log("progress!!!!" + report);
      Recv.set_HTML("tdec", "progress " + report);
      const progress_container = document.getElementById('progress_bars');
      const query = '#progress_bars > div[class="progress"]';
      const prev = document.querySelectorAll(query);
      // el.remove() iff report len is shorter than current

      if (!prev || prev.length < report.length) {
        for (var i = (prev ? prev.length : 0); i < report.length; i++) {
          var aaa = document.createElement('div');
          aaa.classList.add("progress");
          progress_container.appendChild(aaa);
        }
      }
      else if (report.length < prev.length) {
        for (var i = report.length; i < prev.length; i++) {
          prev[i].remove();
        }
      }

      const current = document.querySelectorAll(query);
      if (current) {
        console.log(current.length);
      }
      for (var i = 0; i < report.length; i++) {
        console.log(report[i] * 100 + "%");
        current[i].style.width = report[i] * 100 + "%";
      }
    },

    toggleFullscreen: function () {
      // nothin yet
    },

    showDebug: function () {
      document.getElementById("debug-button").focus();
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
    },

    setMode: function (mode_str) {
      const modeVal = (mode_str == "4C") ? 4 : 68;
      Module._cimbard_configure_decode(255, modeVal);
      for (let i = 0; i < _workers.length; i++) {
        // cal config decode within the workers as well
        _workers[i].postMessage({ config: true, color_bits: 255, mode_val: modeVal });
      }

      var nav = document.getElementById("nav-container");
      if (mode_str == "4C") {
        nav.classList.remove("mode-b");
        nav.classList.add("mode-4c");
      } else if (mode_str == "B") {
        nav.classList.add("mode-b");
        nav.classList.remove("mode-4c");
      } else {
        nav.classList.remove("mode-b");
        nav.classList.remove("mode-4c");
      }
    },

    set_HTML: function (id, msg, only_if_unset) {
      const elem = document.getElementById(id);
      if (only_if_unset && elem.innerHTML) {
        return;
      }
      elem.innerHTML = msg;
    },

    set_title: function (msg) {
      document.title = "Cimbar: " + msg;
    }
  };
}();

