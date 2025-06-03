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
		    console.log(vf);
			// malloc iff necessary
		    DecWorker.mallocAll(vf);
		    vf.copyTo(DecWorker.imgBuff(), {format: "RGBX"});
		    // then decode in wasm, fool
		    var avg = Module._do_decode(DecWorker.imgBuff().byteOffset, width, height);
		    self.postMessage({ type: 'proc', res: avg });
		} catch (e) {
		    console.log(e);
		}
		vf.close();
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
