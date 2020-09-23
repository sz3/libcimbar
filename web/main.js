var Main = function() {

var _interval = 100;

function importFile(f)
{
  const fileReader = new FileReader();
  fileReader.onload = (event) => {
    const imageData = new Uint8Array(event.target.result);
    const numBytes = imageData.length * imageData.BYTES_PER_ELEMENT;
    const dataPtr = Module._malloc(numBytes);
    const dataOnHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numBytes);
    dataOnHeap.set(imageData);
    Main.encode(f.name, dataOnHeap);
    Module._free(dataPtr);
  };
  fileReader.onerror = () => {
    console.error('Unable to read file ' + f.name + '.');
  };

  fileReader.readAsArrayBuffer(f);
}

// public interface
return {
  run : function(container, canvas)
  {
    console.log("init for canvas " + container);
    Module._initialize_GL(1024, 1024);
  },

  encode : function(filename, data)
  {
    console.log("encoding " + filename);
    var res = Module._encode(data.byteOffset, data.length);
    console.log(res);
    Main.setHTML("status", filename + ":" + res);
  },

  dragDrop : function(event)
  {
    console.log("drag drop?");
    console.log(event);
    const files = event.dataTransfer.files;
    if (files && files.length === 1) {
      importFile(files[0]);
    }
  },

  fileInput : function(ev)
  {
    console.log(ev);
    var file = document.getElementById('file_input').files[0];
    if (file)
       importFile(file);
  },

  nextFrame : function()
  {
    //console.log("draw frame, interval " + _interval);
    setTimeout(Main.nextFrame, _interval);
    Module._render();
  },

  setHTML : function(id, msg)
  {
    document.getElementById(id).innerHTML = msg;
  }
};
}();

window.addEventListener("dragover", function(e) {
  e = e || event;
  e.preventDefault();
}, false);

window.addEventListener("drop", function(e) {
  e = e || event;
  e.preventDefault();
  e.stopPropagation();
  Main.dragDrop(e);
}, false);
