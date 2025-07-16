var Zstd = function() {

var _counter = 0;
var _activeBuff = undefined;
var _decompBuff = undefined;

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

function importFile(f)
{
  const fileReader = new FileReader();
  fileReader.onload = (event) => {
    const fileData = new Uint8Array(event.target.result);

	Zstd.decompress(f.name, fileData);
    Zstd.set_HTML("errorbox", f.name);
  };
  fileReader.onerror = () => {
    console.error('Unable to read file ' + f.name + '.');
  };

  fileReader.readAsArrayBuffer(f);
}

function allocBuff(data)
{
	freeBuff();
	const numBytes = data.length * data.BYTES_PER_ELEMENT;
    const dataPtr = Module._malloc(numBytes);
    _activeBuff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numBytes);
    _activeBuff.set(data);
}

function freeBuff()
{
	if (_activeBuff)
		Module._free(_activeBuff.byteOffset);
	_activeBuff = undefined;
}

function getDecompressReader(inbuff)
{
	// allocate buffer once. We'll reuse it,
	// and slice() to copy to the local (non-wasm) heap
	const bufferSize = Module._cimbarz_get_bufsize(); // chunk size
	if (_decompBuff === undefined)
		_decompBuff = Module._malloc(bufferSize);

	const readstream = new ReadableStream({
	  start(controller) {
		const res = Module._cimbarz_init_decompress(inbuff.byteOffset, inbuff.length);
		if (res < 0)
		{
			console.log("init decompress failed, res " + res);
		}
	  },
	  pull(controller) {
		  const bytesRead = Module._cimbarz_decompress_read(_decompBuff, bufferSize);
			console.log("reader got " + bytesRead);
		  if (bytesRead <= 0) {
		    // No more data, close the stream
		    controller.close();
		  } else {
		    // Copy the data from WASM memory to a JavaScript Uint8Array
		    const chunk = new Uint8Array(Module.HEAPU8.buffer, _decompBuff, bytesRead).slice();
		    controller.enqueue(chunk);
		  }
	  },
	  cancel() {
		// Handle cancellation if needed
		}
	});
	return readstream;
}

async function saveViaStreaming(readstream, filename)
{
  try {
    // 1. Prompt user for a file location.
    const fileHandle = await window.showSaveFilePicker({
      suggestedName: filename,
    });

    // 2. Create a writable stream directly to the file.
    const writableStream = await fileHandle.createWritable();

    // 3. Pipe the data from your source to the file on disk.
    await readstream.pipeTo(writableStream);

    console.log('File saved successfully!');
  } catch (error) {
    // Handle errors, like the user canceling the dialog.
    console.error('File save failed:', error);
  }
}

async function saveViaBlob(readstream, filename) {
  // Create a new Response object from the stream
  const response = new Response(readstream);

  // Create a blob from the response
  const blob = await response.blob();

  // Create an object URL from the blob
  const url = URL.createObjectURL(blob);

  // Create a link element and simulate a click to download
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

function saveToFile(readstream, filename)
{
	if (window.showSaveFilePicker == undefined)
		saveViaBlob(readstream, filename);
	else
		saveViaStreaming(readstream, filename);
}

// public interface
return {

  set_error : function(msg)
  {
    Main.set_HTML('errorbox', msg);
    return false;
  },

  clickFileInput : function()
  {
    document.getElementById("file_input").click();
  },

  fileInput : function(ev)
  {
    console.log("file input: " + ev);
    var file = document.getElementById('file_input').files[0];
    if (file)
       importFile(file);
  },

  decompress : function(name, data)
  {
	allocBuff(data);

	const reader = getDecompressReader(_activeBuff);

	saveToFile(reader, name);
  },

  get_file : function()
  {
	const size = 15000;
	const dataPtr = Module._malloc(size);
    const res = Module._fake_download(dataPtr, size);
	if (res <= 0) {
		console.log("fake download failed, feelsbadman: " + res);
		return;
	}
	
	const buff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, res);
	console.log(buff);
	alert('we did it ' + res);

	try {
		Main.download_bytes(buff, "0.1234");
	} catch (e) {
		console.log(e);
	}
	
	Module._free(dataPtr);
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

