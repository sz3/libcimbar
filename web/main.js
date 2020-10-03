var Main = function() {

var _interval = 70;

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
    if (dim > 1040) {
      dim = 1040;
    }
    Module._initialize_GL(1040, 1040);
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
    //console.log("draw frame, interval " + _interval);
    Module._render();
    setTimeout(Main.nextFrame, _interval);
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
}, false);

window.addEventListener("drop", function(e) {
  e = e || event;
  e.preventDefault();
  e.stopPropagation();
  Main.dragDrop(e);
}, false);
