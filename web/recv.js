var Sink = function () {

  var _fountainBuff = undefined;
  var _errBuff = undefined;
  var _errBuffSize = 1024;

  // public interface
  return {
    allocate: function () {
      const size = Module._cimbard_get_bufsize(); // max length of buff. We could also resize as we go...
      if (_fountainBuff && size > _fountainBuff.length) {
        Module._free(_fountainBuff.byteOffset);
        _fountainBuff = undefined;
      }
      if (_fountainBuff === undefined) {
        const dataPtr = Module._malloc(size);
        _fountainBuff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
      }
    },

    on_decode: function (buff) {
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
      if (_errBuff === undefined) {
        _errBuff = Module._malloc(_errBuffSize);
      }
      const errlen = Module._cimbard_get_report(_errBuff, _errBuffSize);
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
      try {
        var name = id + "." + size;
        const fnsize = Module._cimbard_get_filename(id, _errBuff, _errBuffSize);
        if (fnsize < 0) {
          alert("reassemble_file failed :(" + res);
          console.log("we biffed it. :( " + res);
          Recv.set_HTML("errorbox", "reassemble_file failed :( " + res);
        }
        else if (fnsize > 0) {
          const temparr = new Uint8Array(Module.HEAPU8.buffer, _errBuff, fnsize);
          name = new TextDecoder("utf-8").decode(temparr);
        }
        Zstd.decompress(name, id);
      } catch (error) {
        console.log("failed finish copy or download?? " + error);
      }
    }
  };
}();


var Recv = function () {

  var _counter = 0;
  var _recentDecode = -1;
  var _recentExtract = -1;
  var _renderTime = 0;
  var _captureNextFrame = 0;

  var _watchmanEnabled = 0;
  var _watchmanLastSeen = 1; // start at 1, can't restart if we never started

  var _video = 0;
  var _videoTrack = undefined;

  var _workers = [];
  var _nextWorker = 0;
  var _workerReady;
  var _framesInFlight = 0;
  var _supportedFormats = ["NV12", "I420"]; // have cimbard_* return this somehow?

  var _mode = 0;

  function _toggleFullscreen() {
    if (document.fullscreenElement) {
      return document.exitFullscreen();
    }
    else {
      return document.documentElement.requestFullscreen();
    }
  }

  function isIOS() {
    const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent) && !window.MSStream;
    const isAppleDevice = navigator.userAgent.includes('Macintosh');
    const isTouchScreen = navigator.maxTouchPoints >= 1;
    return isIOS || (isAppleDevice && isTouchScreen);
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

    frames_in_flight_incr: function () {
      _framesInFlight += 1;
      document.getElementById('framesInFlight').innerHTML = _framesInFlight;
    },

    frames_in_flight_decr: function () {
      _framesInFlight -= 1;
      document.getElementById('framesInFlight').innerHTML = _framesInFlight;
    },

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
          frameRate: { ideal: 15 }, // we're not trying to set the user's phone on fire
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

          _videoTrack = localMediaStream.getVideoTracks()[0];
          Recv.enable_manual_focus();
          Recv.enable_manual_exposure();
        })
        .catch(err => {
          console.error(`OH NO!!!!`, err);
          Recv.set_error("Failed to initialize camera. " + err);
          Recv.set_HTML("crosshair1", "Failed to initialize camera. " + err);
        });
    },

    enable_manual_focus: function () {
      if (!_videoTrack) {
        console.log("uh oh, no videotrack");
        return;
      }
      const capabilities = _videoTrack.getCapabilities();
      const settings = _videoTrack.getSettings();

      let slider = document.getElementById('focus-slider');
      //console.log(JSON.stringify(capabilities));
      //console.log(capabilities);
      //console.log(settings);
      slider.min = capabilities.focusDistance.min;
      slider.max = capabilities.focusDistance.max;
      slider.step = capabilities.focusDistance.step;
      slider.value = settings.focusDistance;
    },

    enable_manual_exposure: function () {
      if (!_videoTrack) {
        return;
      }
      const capabilities = _videoTrack.getCapabilities();
      const settings = _videoTrack.getSettings();

      let slider = document.getElementById('exposure-slider');
      console.log(capabilities);
      console.log(settings);
      slider.min = capabilities.exposureTime.min;
      slider.max = capabilities.exposureTime.max;
      slider.step = capabilities.exposureTime.step;
      slider.value = settings.exposureTime;
    },

    watch_for_camera_pause: function () {
      // only call this after our first success
      if (_watchmanEnabled) {
        return;
      }
      _watchmanEnabled = true;

      // ios only for now, since desktop behavior is weird
      if (!isIOS()) {
        return;
      }

      // periodically make sure the camera capture is running
      setInterval(Recv.restart_paused_camera, 1000);
    },

    restart_paused_camera: function () {
      if (!_video) {
        return;
      }

      // if we're still incrementing, do nothing
      if (_counter > _watchmanLastSeen) {
        _watchmanLastSeen = _counter;
        return;
      }

      // if not, we're stuck?
      Recv.init_video(_video);
    },

    download_bytes: function (buff, name) {
      var blob = new Blob([buff], { type: 'application/octet-stream' });
      Zstd.download_blob(name, blob);
    },

    on_decode: function (wid, data) {
      //console.log('Main thread received message from worker' + wid + ':', data);
      Recv.frames_in_flight_decr();
      // if extract but no bytes, log extract counte
      if (data.nodata) {
        _recentExtract = _counter;
        return;
      }
      if (data.failed_extract) { // very common, nothing to do
        return;
      }
      if (data.res) {
        Recv.set_HTML("t" + wid, "msg is " + data.res);
        return;
      }
      if (data.ready) {
        if (_workerReady)
          _workerReady();
        return;
      }

      // should be a decode with some bytes, so set decodecounter
      _recentDecode = _counter;

      const buff = data.buff;
      if (buff.length > 0) {
        Recv.setMode(data.mode); // call *before* we send it to the sink. This is our autodetect confirm.
      }
      Recv.set_HTML("t" + wid, "mode is " + _mode + ", len() is " + buff.length + ", buff: " + buff);
      Sink.on_decode(buff);
    },

    on_frame: function (now, metadata) {
      //console.log("on frame");
      // https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame

      _counter += 1;
      if (_workers.length == 0)
        return;
      if (_nextWorker >= _workers.length)
        _nextWorker = 0;

      // piggyback off this call to make sure our visual state is correct
      Recv.update_visual_state();
      // make sure the camera feed stays up
      Recv.watch_for_camera_pause();

      const modeVals = [67, 68, 69, 4];

      var vf = undefined;
      if (_framesInFlight > 20) {
        console.log("stalling, worker queues are full");
      }
      else {
        Recv.frames_in_flight_incr();
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
            Recv.download_bytes(buff, width + "x" + height + "x" + _counter + "." + format);
          }

          let mode = _mode || modeVals[_counter % modeVals.length];
          _workers[_nextWorker].postMessage({ type: 'proc', pixels: buff, format: format, width: width, height: height, mode: mode }, [buff.buffer]);
        } catch (e) {
          console.log(e);
        }
        _nextWorker += 1;
      }
      if (vf)
        vf.close();

      // schedule the next one
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

    update_visual_state: function () {
      // check counters
      var xh1 = document.getElementById("crosshair1");
      var xh2 = document.getElementById("crosshair2");
      if (_recentDecode > 0 && _recentDecode + 30 > _counter) {
        xh1.classList.add("active_xhairs");
        xh1.classList.remove("scanning_xhairs");
        xh2.classList.add("active_xhairs");
        xh1.classList.remove("scanning_xhairs");
      }
      else if (_recentExtract > 0 && _recentExtract + 30 > _counter) {
        xh1.classList.add("scanning_xhairs");
        xh1.classList.remove("active_xhairs");
        xh2.classList.add("scanning_xhairs");
        xh2.classList.remove("active_xhairs");
      }
      else { // inactive
        xh1.classList.remove("active_xhairs");
        xh1.classList.remove("scanning_xhairs");
        xh2.classList.remove("active_xhairs");
        xh2.classList.remove("scanning_xhairs");
      }
    },

    render_progress: function (report) {
      console.log("progress!!!!" + report);
      Recv.set_HTML("tdec", "progress " + report);
      const progress_container = document.getElementById('progress_bars');
      const query = '#progress_bars > div[class="progress"]';
      const prev = document.querySelectorAll(query);

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

    updateFocus: async function (val) {
      console.log("focus is " + val);
      Recv.set_HTML("crosshair2", "focus " + val);
      if (!_videoTrack) {
        return;
      }
      console.log("setting focus, maybe");
      await _videoTrack.applyConstraints({
        advanced: [{ focusMode: 'manual', focusDistance: val }]
      });
    },

    updateExposure: async function (val) {
      console.log("exposure is " + val);
      Recv.set_HTML("crosshair2", "exposure " + val);
      if (!_videoTrack) {
        return;
      }
      console.log("setting exposure, maybe");
      await _videoTrack.applyConstraints({
        advanced: [{ exposureMode: 'manual', exposureTime: val }]
      });
    },

    toggleFullscreen: function () {
      _toggleFullscreen();
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

    setMode: function (modeVal) {
      // these should be moved elsewhere...
      const modeToString = {
        4: "4C",
        8: "8C",
        67: "Bm",
        68: "B",
        69: "Bw"
      };
      let modeStringToVal = {
        "Auto": 0
      };
      for (const val in modeToString) {
        modeStringToVal[modeToString[val]] = val;
      }

      if (modeVal in modeStringToVal) {
        modeVal = modeStringToVal[modeVal];
      }

      // configure wasm in main thread
      _mode = modeVal;
      if (_mode > 0) {
        Module._cimbard_configure_decode(_mode);
        Sink.allocate();
      }

      // update ui
      if (_mode > 0) {
        var nav = document.getElementById("mode-val");
        nav.innerHTML = modeToString[_mode];
      }

      var nav = document.getElementById("nav-container");
      if (_mode == 0) {
        nav.classList.add("mode-auto");
        nav.classList.remove("mode-b");
      } else {
        nav.classList.add("mode-b");
        nav.classList.remove("mode-auto");
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

