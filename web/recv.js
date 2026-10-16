var Sink = function () {

  var _fountainBuff = undefined;
  var _errBuff = undefined;
  var _errBuffSize = 1024;

  function fountain_buff() {
    if (_fountainBuff.buffer !== Module.HEAPU8.buffer) {
      _fountainBuff = new Uint8Array(Module.HEAPU8.buffer, _fountainBuff.byteOffset, _fountainBuff.byteLength);
    }
    return _fountainBuff;
  }

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
      const fountBuff = fountain_buff();
      fountBuff.set(buff);

      //console.log('sink decode ' + fountBuff); //TODO: base64?
      var res = Module._cimbard_fountain_decode(fountBuff.byteOffset, buff.length);
      console.log("on decode got res " + res);

      const report = Sink.get_report();
      if (Array.isArray(report)) {
        Recv.render_progress(report);
      }
      else {
        Recv.set_HTML("tdec", "decode " + res + ". " + report);
      }

      if (res > 0) {
        const res32t = Number(res & 0xFFFFFFFFn); // truncate BigInt res (int64_t) to a uint32_t
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
          console.log("reassemble_file failed. :( " + fnsize);
          Recv.set_HTML("errorbox", "reassemble_file failed :( " + fnsize);
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

  var _watchmanTimer = undefined;
  var _watchmanLastSeen = 1; // start at 1, can't restart if we never started

  var _video = 0;
  var _useVideoFrame = true;
  var _vfcId = undefined;

  var _workers = [];
  var _nextWorker = 0;
  var _workerReady;
  var _framesInFlight = 0;
  var _supportedFormats = ["NV12", "I420"]; // have cimbard_* return this somehow?

  var _mode = 0;

  function _requestVideoFrame(callback) {
    if ('requestVideoFrameCallback' in _video) {
      _vfcId = _video.requestVideoFrameCallback(callback);
    } else {
      _vfcId = requestAnimationFrame(function (now) {
        return callback(now, {});
      });
    }
  }

  function _cancelVideoFrameCallback() {
    if (_video && _vfcId !== undefined) {
      if ('cancelVideoFrameCallback' in _video) {
        _video.cancelVideoFrameCallback(_vfcId);
      } else {
        cancelAnimationFrame(_vfcId);
      }
      _vfcId = undefined;
    }
  }

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

  function _getModeAspectRatio(mode) {
    // (image_size_x + 16) / (image_size_y + 16)
    switch (mode) {
      case 66: return 1.1516; // Bu
      case 67: return 1.413;  // Bm
      default: return 1.0;    // B, 4C, auto
    }
  }

  function _updateCrosshairPositions() {
    if (!_video || !_video.videoWidth || !_video.videoHeight)
      return;

    var modeAspect = _getModeAspectRatio(_mode);

    var windowW = window.innerWidth;
    var windowH = window.innerHeight;
    var camAspect = _video.videoWidth / _video.videoHeight;
    var windowAspect = windowW / windowH;

    var vidW = windowW;
    var vidH = windowH;
    if (camAspect > windowAspect)  // black bars top/bottom
      vidH = vidW / camAspect;
    else  // black bars left/right
      vidW = vidH * camAspect;

    var offsetY;
    var offsetX;
    if (windowH > windowW) {
      // portrait
      offsetY = (windowH - (vidW * modeAspect)) / 2;
      offsetX = (windowW - vidW) / 2;
    }
    else {
      offsetY = (windowH - vidH) / 2;
      offsetX = (windowW - (vidH * modeAspect)) / 2;
    }

    //var logme = "crosshair offsets now " + offsetX + ", " + offsetY;
    //Recv.set_error(logme);
    //console.log(logme);

    var xh1 = document.getElementById("crosshair1");
    var xh2 = document.getElementById("crosshair2");
    xh1.style.top = offsetY + "px";
    xh1.style.right = offsetX + "px";
    xh2.style.bottom = offsetY + "px";
    xh2.style.left = offsetX + "px";
  }

  // public interface
  return {
    init: function (video, num_workers) {
      Recv.init_ww(num_workers);
      Recv.init_camera(video);
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
      document.getElementById('framesInFlight').innerHTML = _framesInFlight + "  " + _useVideoFrame;
    },

    frames_in_flight_decr: function () {
      _framesInFlight -= 1;
      document.getElementById('framesInFlight').innerHTML = _framesInFlight + "  " + _useVideoFrame;
    },

    init_ww: function (num_workers) {
      //num_workers = 1;
      // clean up _workers if exists
      for (const wkr of _workers) {
        try {
          wkr.terminate();
        } catch (e) { }
      }
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

    init_camera: function (video) {
      _video = video;
      window.addEventListener('resize', _updateCrosshairPositions); //idempotent, I think

      var constraints = {
        audio: false,
        video: {
          width: { min: 720, ideal: 1920 },
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
          Recv.start_video(localMediaStream);
        })
        .catch(err => {
          console.error(`OH NO!!!!`, err);
          Recv.set_error("Failed to initialize camera. " + err);
          Recv.set_HTML("crosshair1", "Failed to initialize camera. " + err);
        });
    },

    start_video: function (vidSource) {
      if (!_video) {
        return;
      }

      Recv.stop_video(); // clean up

      if (vidSource instanceof MediaStream) {
        if ('srcObject' in _video) {
          _video.srcObject = vidSource;
        }
        else {
          _video.src = URL.createObjectURL(vidSource); // deprecated?
        }
      }
      else {
        // file or object url (for e.g. mp4)
        _video.srcObject = null;
        _video.src = typeof vidSource === 'string' ? vidSource : URL.createObjectURL(vidSource);
      }

      _video.play().then(() => {
        _requestVideoFrame(Recv.on_frame);
      }).catch(err => {
        if (err.name === 'NotAllowedError') {
          console.log("Autoplay blocked...");
          // TODO: getPermissionSomehow();
          // then call _requestVideoFrame in callback
        }
        console.error("Video play blocked or failed:", err);
        Recv.set_error("Playback failed: " + err);
      });
    },

    stop_video: function () {
      _cancelVideoFrameCallback();
      if (_video && _video.srcObject) {
        _video.srcObject.getTracks().forEach(track => track.stop());
        _video.srcObject = null;
      }
      if (_video && _video.src) {
        URL.revokeObjectURL(_video.src);
        _video.src = "";
      }

      if (_watchmanTimer > 0) {
        clearInterval(_watchmanTimer);
      }
      _watchmanTimer = 0;
    },

    watch_for_camera_pause: function () {
      // only call this after our first success
      if (_watchmanTimer) {
        return;
      }

      // periodically make sure the camera capture is running
      _watchmanTimer = setInterval(Recv.restart_paused_camera, 1500);
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
      clearInterval(_watchmanTimer); // our work is done, for now
      _watchmanTimer = 0;
      Recv.init_camera(_video);
    },

    download_bytes: function (buff, name) {
      var blob = new Blob([buff], { type: 'application/octet-stream' });
      Zstd.download_blob(name, blob);
    },

    on_decode: function (wid, data) {
      //console.log('Main thread received message from worker' + wid + ':', data);
      if (data.type == 'startWasm') {
        if (data.ready && _workerReady) {
          _workerReady();
        }
        return;
      }

      Recv.frames_in_flight_decr();
      // if extract but no bytes, log extract counter
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
      if (data.error) {
        // other error cases
        console.log("on_decode error? " + data.error);
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

    on_frame_vf: async function (now) {
      // the normal path
      let vf = undefined;
      try {
        vf = new VideoFrame(_video, { timestamp: now });
      } catch (e) {
        console.log("no VideoFrame!");
        return undefined;
      }

      try {
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
        await vf.copyTo(buff, vfparams);

        let format = vfparams.format || vf.format;
        if (format == "RGBA" && size != width * height * 4) {
          format = vf.format; //fallback
        }

        // debugging only
        if (_captureNextFrame == 1) {
          _captureNextFrame = 0;
          Recv.download_bytes(buff, width + "x" + height + "x" + _counter + "." + format);
        }

        let workerMsg = {
          type: 'proc',
          pixels: buff,
          format: format,
          width: width,
          height: height,
        };
        return workerMsg;
      } catch (e) {
        console.error(e);
        return {
          type: 'fail'
        };
      } finally {
        vf.close();
      }
    },

    on_frame: async function (now, metadata) {
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

      const modeVals = [66, 68, 67, 4];

      if (_framesInFlight > 30) {
        console.log("stalling, worker queues are full");
        _requestVideoFrame(Recv.on_frame);
        return;
      }

      let mode = _mode || modeVals[_counter % modeVals.length];
      let bitmap = undefined;

      let workerMsg = undefined;
      if (_useVideoFrame) {
        workerMsg = await Recv.on_frame_vf(now);
        if (!workerMsg) {
          _useVideoFrame = false;
        }
        else if (workerMsg.type == "fail") {
          workerMsg = undefined;
        }
        else {
          workerMsg.mode = mode;
        }
      }
      else {
        // TODO: if videowidth/height not set, bail?
        const width = _video.videoWidth;
        const height = _video.videoHeight;
        bitmap = await createImageBitmap(_video);
        workerMsg = {
          type: 'proc',
          bitmap: bitmap,
          format: "RGBA",
          width: width,
          height: height,
          mode: mode
        };
      }

      if (!workerMsg) {
        _requestVideoFrame(Recv.on_frame);
        return;
      }

      Recv.frames_in_flight_incr();
      try {
        if (workerMsg.pixels) {
          _workers[_nextWorker].postMessage(workerMsg, [workerMsg.pixels.buffer]);
        }
        else {
          _workers[_nextWorker].postMessage(workerMsg, [bitmap]);
        }
        _nextWorker += 1;
      }
      catch (e) {
        Recv.frames_in_flight_decr();
        if (bitmap) {
          bitmap.close();
        }
        throw e; // rethrow
      }

      _requestVideoFrame(Recv.on_frame);
    },

    captureFrame: function () {
      _captureNextFrame = 1;
      console.log("capturing next frame");
    },

    download_bytes: function (buff, name) {
      var blob = new Blob([buff], { type: 'application/octet-stream' });
      Zstd.download_blob(name, blob);
    },

    update_visual_state: function () {
      _updateCrosshairPositions();

      // check counters
      var xh1 = document.getElementById("crosshair1");
      var xh2 = document.getElementById("crosshair2");
      if (_recentDecode > 0 && _recentDecode + 30 > _counter) {
        xh1.classList.add("active_xhairs");
        xh1.classList.remove("scanning_xhairs");
        xh2.classList.add("active_xhairs");
        xh2.classList.remove("scanning_xhairs");
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

    clickFileInput: function () {
      document.getElementById("video_input").click();
    },

    fileInput: function (ev) {
      console.log("video input: " + ev);
      var file = document.getElementById('video_input').files[0];
      if (file) {
        Recv.start_video(file);
      }
      Recv.blurNav(false);
    },

    setMode: function (modeVal) {
      // these should be moved elsewhere...
      const modeToString = {
        4: "4C",
        8: "8C",
        66: "Bu",
        67: "Bm",
        68: "B"
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
