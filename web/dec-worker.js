let _wasmInitialized = false;
let _buffs = {};

var Module = {
    preRun:[],
    onRuntimeInitialized: function load_done_callback() {
        console.info("The Module is loaded and is accessible here", Module);
        _wasmInitialized = true;
    }
};

var DecWorker = function() {

// public interface
return {
	on_frame : function(data)
	{
		const vf = data.vf;
		const width = data.width;
		const height = data.height;
		try {
		    //console.log(vf);
			// malloc iff necessary
		    DecWorker.mallocAll(vf);
			const imgBuff = DecWorker.imgBuff();
		    vf.copyTo(imgBuff, {format: "RGBX"});
		    // then decode in wasm, fool
			const fountainBuff = DecWorker.fountainBuff();
		    var len = Module._scan_extract_decode(imgBuff.byteOffset, width, height, 4,  fountainBuff.byteOffset, fountainBuff.length);

			// copy fountainBuff to msgbuf
			// const msgbuf = new Uint8Array(Module.HEAPU8.buffer, fountainBuff.byteOffset, fountainBuff.length);
			// const tempbuf = msgbuf.buffer;
  			//self.postMessage(msgbuf.buffer, [msgbuf.buffer]);
			if (len <= 0 && len != -3) {
				const errmsg = DecWorker.get_error();
				len += ' ' + errmsg;
				console.log(errmsg);
		    	self.postMessage({ error: true, res: len }); //TODO: send fountainBuff too...
			}
			else if (len > 0) {
				console.log('len is ' + len);
				//self.postMessage({ error: true, res: len });
				const msgbuf = new Uint8Array(Module.HEAPU8.buffer, fountainBuff.byteOffset, fountainBuff.length).slice();
				console.log(msgbuf);
				self.postMessage(msgbuf.buffer, [msgbuf.buffer]);
			}
			// in main, const receivedArray = new Uint8Array(event.data);
		} catch (e) {
		    console.log(e);
		}
		vf.close();
	},

	get_error : function()
	{
		const errbuff = DecWorker.mallocPlease("error", 256);
		const errlen = Module._get_report(errbuff.byteOffset, errbuff.length);
		if (errlen > 0) {
			const errview = new Uint8Array(Module.HEAPU8.buffer, errbuff.byteOffset, errlen);
			const decoder = new TextDecoder();
			return decoder.decode(errview); 
		}
		return "";
	},

  	mallocPlease : function(name, size)
	{
		if (_buffs[name] === undefined || size > _buffs[name].length)
        {
			try {
            	if (size > _buffs[name].length)
				{
					console.log("resizing " + name + " buff from " + _buffs[name].length + " to " + size);
					Module._free(_buffs[name].byteOffset);
				}
			} catch (e) {} // if we're leaking memory we'll find out the hard way 
            const dataPtr = Module._malloc(size);
            _buffs[name] = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
        }
		return _buffs[name];
	},

	mallocAll : function(vf)
	{
		const imgsize = vf.allocationSize({format: "RGBX"});
		DecWorker.mallocPlease("img", imgsize);

		const bufsize = Module._fountain_get_bufsize();
		DecWorker.mallocPlease("fountain", bufsize);
	},

	imgBuff : function()
	{
		return _buffs['img'];
	},

	fountainBuff : function()
	{
		return _buffs['fountain'];
	}
};
}();

importScripts('cimbar_js.js');

self.onmessage = async (event) => {

	if (!_wasmInitialized)
	{
       	console.log('we got no wasm :(');
		self.postMessage({ type: 'startWasm', error: "no wasm" });
		return;
	}

	if (event.data.config)
	{
		console.log('attempting config');
		try {
			Module._configure_decode(event.config.color_bits, event.config.mode_val);
		} catch (e) {
			self.postMessage({ type: 'config', error: e });
		}
		return;
	}
	
	// assert 'vf' in data?
	DecWorker.on_frame(event.data)
};
