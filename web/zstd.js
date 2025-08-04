var Zstd = function () {

  var _activeBuff = undefined;
  var _decompBuff = undefined;

  function allocBuff(data) {
    freeBuff();
    const numBytes = data.length * data.BYTES_PER_ELEMENT;
    const dataPtr = Module._malloc(numBytes);
    _activeBuff = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numBytes);
    _activeBuff.set(data);
  }

  function freeBuff() {
    if (_activeBuff)
      Module._free(_activeBuff.byteOffset);
    _activeBuff = undefined;
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

  function getDecompressReader(inbuff) {
    // allocate buffer once. We'll reuse it,
    // and slice() to copy to the local (non-wasm) heap
    const bufferSize = Module._cimbarz_get_bufsize(); // chunk size
    if (_decompBuff === undefined)
      _decompBuff = Module._malloc(bufferSize);

    const readstream = new ReadableStream({
      start(controller) {
        const res = Module._cimbarz_init_decompress(inbuff.byteOffset, inbuff.length);
        if (res < 0) {
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

  async function saveViaStreaming(readstream, filename) {
    // afaik there's no way to get the save file picker to activate
    // without user input? Which ... sucks?
    alert("download ready!");
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
    // Create Response object from the stream, get blob
    const response = new Response(readstream);
    const blob = await response.blob();
    Zstd.download_blob(filename, blob);
  }

  function saveToFile(readstream, filename) {
    ///if (window.showSaveFilePicker == undefined)
    saveViaBlob(readstream, filename);
    //else
    //  saveViaStreaming(readstream, filename);
  }

  // public interface
  return {

    // helper
    download_blob: function (name, blob) {
      var bloburl = window.URL.createObjectURL(blob);
      _downloadHelper(name, bloburl);
      setTimeout(function () {
        return window.URL.revokeObjectURL(bloburl);
      }, 1000);
    },

    decompress: function (name, data) {
      allocBuff(data);

      const reader = getDecompressReader(_activeBuff);

      saveToFile(reader, name);
    }
  };
}();
