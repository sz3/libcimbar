let _wasmInitialized = false;
let _buff = undefined;

var Module = {
    preRun:[],
    onRuntimeInitialized: function load_done_callback() {
        console.info("The Module is loaded and is accessible here", Module);
        _wasmInitialized = true;
    },
};

importScripts('cimbar_js.js');

self.onmessage = async (event) => {

	if (!_wasmInitialized)
	{
       	console.log('we got no wasm :(');
		self.postMessage({ type: 'startWasm', error: "no wasm" });
		return;
	}
	
	const vf = event.data.vf;
	const width = event.data.width;
	const height = event.data.height;
	try {
        console.log(vf);
        if (_buff === undefined)
        {
            const size = vf.allocationSize({format: "RGBX"});
            const dataPtr = Module._malloc(size);
            _buff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, size);
        }
        vf.copyTo(_buff, {format: "RGBX"});
        // then decode in wasm, fool
        var avg = Module._do_decode(_buff.byteOffset, width, height);
        self.postMessage({ type: 'proc', res: avg });
    } catch (e) {
        console.log(e);
    }
	vf.close();
};
