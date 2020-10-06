var Main = function() {

var _interval = 66;

var _showStats = true;
var _renders = 0;
var _renderTime = 0;

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
  run : function(canvas)
  {
    console.log("init for canvas " + canvas);
    var dim = canvas.width;
    if (canvas.height < dim) {
      dim = canvas.height;
    }
    console.log(dim);
    if (dim > 1024) {
      dim = 1024;
    }
    Module._initialize_GL(1024, 1024);
    canvas.style.width = dim + "px";
    canvas.style.height = dim + "px";
  },

  encode : function(filename, data)
  {
    console.log("encoding " + filename);
    var res = Module._encode(data.byteOffset, data.length);
    console.log(res);
    Main.setTitle(filename);
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
    var start = performance.now();
    var renderCount = Module._render();

    var elapsed = performance.now() - start;
    var nextInterval = _interval>elapsed? _interval-elapsed : 0;
    setTimeout(Main.nextFrame, nextInterval);

    if (_showStats && renderCount) {
      _renderTime += elapsed;
      Main.setHTML( "status", elapsed + " : " + renderCount + " : " + Math.ceil(_renderTime/renderCount));
    }
  },

  setHTML : function(id, msg)
  {
    document.getElementById(id).innerHTML = msg;
  },

  setTitle : function(msg)
  {
    document.title = "Cimbar: " + msg;
  }
};
}();

window.addEventListener("dragover", function(e) {
  e = e || event;
  e.preventDefault();

  document.body.style["opacity"] = 0.5;
}, false);

window.addEventListener("dragleave", function(e) {
  e = e || event;
  e.preventDefault();

  document.body.style["opacity"] = 1.0;
}, false);

window.addEventListener("drop", function(e) {
  e = e || event;
  e.preventDefault();
  e.stopPropagation();
  Main.dragDrop(e);
  document.body.style["opacity"] = 1.0;
}, false);
