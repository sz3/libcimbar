var Sink = function() {

var _fountainBuff = undefined;

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
	Dec.set_HTML("tdec", "decode " + res);
	if (res > 0) {
		Sink.reassemble_file();
	}
  },

  reassemble_file : function()
  {
	alert("we did it!?!");
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

function toggleFullscreen()
{
  if (document.fullscreenElement) {
    return document.exitFullscreen();
  }
  else {
    return document.documentElement.requestFullscreen();
  }
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
    // ios nonsense
    video.setAttribute('autoplay', '');
    video.setAttribute('muted', '');
    video.setAttribute('playsinline', '');

    var constraints = {
        audio: false,
        video: {
          	facingMode: 'environment',
            width: { ideal: 1080 }, // Request HD but allow flexibility
            height: { ideal: 1080 },
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

	try {
		const vf = new VideoFrame(_video);
		_workers[_nextWorker].postMessage({ type: 'proc', vf: vf, width: _video.videoWidth, height: _video.videoHeight });
	} catch (e) {
        console.log(e);
    }

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

