var Sink = function() {

var _fountainBuff = undefined;
var _errBuff = undefined;

// public interface
return {
  on_decode : function(buff)
  {
	if (_fountainBuff === undefined) {
		const size = Module._fountain_get_bufsize(); // max length of buff. We could also resize as we go...
		const dataPtr = Module._malloc(size);
		_fountainBuff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
	}
	_fountainBuff.set(buff);
	var res = Module._fountain_decode(_fountainBuff.byteOffset, buff.length);
	Dec.set_HTML("tdec", "decode " + res + ". " + Sink.get_report());
	if (res > 0) {
		Sink.reassemble_file(res);
	}
  },

  get_report : function()
  {
	const maxSize = 1024;
	if (_errBuff === undefined) {
		_errBuff = Module._malloc(maxSize);
	}
	const errlen = Module._get_report(_errBuff, maxSize);
	if (errlen > 0) {
		const errview = new Uint8Array(Module.HEAPU8.buffer, _errBuff, errlen);
		const decoder = new TextDecoder();
		return decoder.decode(errview); 
	}
  },

  reassemble_file : function(id)
  {
	const size = Module._fountain_get_filesize(id);
	alert("we did it!?! " + size);
	const dataPtr = Module._malloc(size);
	const buff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
	try {
		var res = Module._fountain_finish_copy(id, buff.byteOffset, buff.length);
		if (res < 0) {
			alert("we biffed it. :( " + res);
			console.log("we biffed it. :( " + res);
			Dec.set_HTML("errorbox", "reassemble_file failed :( " + res);
			return;
		}
		console.log("we did it fr fr");
		alert("it's done! " + size);
		// may need to slice() buff here to copy from wasm...
		Dec.download_bytes(buff, size + ".zst"); // size -> name, eventually
	} catch (error) {
		Module._free(dataPtr);
	}
  }
};
}();


var Dec = function() {

var _counter = 0;
var _renderTime = 0;
var _video = 0;

var _workers = [];
var _nextWorker = 0;

var _fountainBuff = undefined;

function _toggleFullscreen()
{
  if (document.fullscreenElement) {
    return document.exitFullscreen();
  }
  else {
    return document.documentElement.requestFullscreen();
  }
}

function _downloadHelper(name, bloburl)
{
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
  init : function(video, num_workers)
  {
    Dec.init_video(video);
	Dec.init_ww(num_workers);
  },

  set_error : function(msg)
  {
    Dec.set_HTML('errorbox', msg);
    return false;
  },

  init_ww : function(num_workers)
  {
	// clean up _workers if exists?
	_workers = [];
	for (let i = 0; i < num_workers; i++)
	{
		_workers.push(new Worker('dec-worker.js'));

		_workers[i].onmessage = (event) => {
			Dec.on_decode(i, event.data);
		};

		_workers[i].onerror = (error) => {
			console.error('Worker' + i + ' error:', error);
		};
	}
  },

  init_video : function(video)
  {
    _video = video;

    var constraints = {
		audio: false,
		video: {
			width: { min: 720, ideal: 1920 }, // Request HD but allow flexibility
			height: { min: 720, ideal: 1080 },
			aspectRatio: matchMedia( 'all and (orientation:landscape)' ).matches ? 16/9 : 9/16,
			facingMode: 'environment',
			exposureMode: 'continuous',
			focusMode: 'continuous',
		}
	};

    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia)
    {
        return Dec.set_error('mediaDevices not supported? :(');
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
      video.requestVideoFrameCallback(Dec.on_frame);
    })
    .catch(err => {
      console.error(`OH NO!!!!`, err);
    });

  },

  download_bytes : function(buff, name)
  {
	var blob = new Blob([buff], {type: 'application/octet-stream'});
	var bloburl = window.URL.createObjectURL(blob);
	_downloadHelper(name, bloburl);
    setTimeout(function() {
		return window.URL.revokeObjectURL(bloburl);
	}, 1000);
  },

  on_decode : function(wid, data)
  {
	console.log('Main thread received message from worker' + wid + ':', data);
	if (data.res) {
		Dec.set_HTML("t" + wid, "avg red " + wid + " is " + data.res);
		return;
	}

	const buff = new Uint8Array(data);
	Dec.set_HTML("t" + wid, "len() is " + buff.length + ", buff: " + buff);
	Sink.on_decode(buff);
  },

  on_frame : function(now, metadata)
  {
    //console.log("on frame");
    // https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame

	if (_workers.length == 0)
		return;
	if (_nextWorker >= _workers.length)
		_nextWorker = 0;

	var vf = undefined;
	try {
		vf = new VideoFrame(_video);
		const size = vf.allocationSize({format: "RGBX"});
		const buff = new Uint8Array(size);
		vf.copyTo(buff, {format: "RGBX"});
		_workers[_nextWorker].postMessage({ type: 'proc', pixels: buff, width: _video.videoWidth, height: _video.videoHeight });
	} catch (e) {
        console.log(e);
    }
	if (vf)
		vf.close();

    // schedule the next one
	_nextWorker += 1;
    _video.requestVideoFrameCallback(Dec.on_frame);
  },

  set_HTML : function(id, msg)
  {
    document.getElementById(id).innerHTML = msg;
  },

  set_title : function(msg)
  {
    document.title = "Cimbar: " + msg;
  }
};
}();

