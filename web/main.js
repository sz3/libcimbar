var Main = function() {

var _interval = 66;

var _showStats = false;
var _renders = 0;
var _renderTime = 0;

function toggleFullscreen()
{
  if (document.fullscreenElement) {
    return document.exitFullscreen();
  }
  else {
    return document.documentElement.requestFullscreen();
  }
}

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

    Main.setHTML("current-file", f.name);
  };
  fileReader.onerror = () => {
    console.error('Unable to read file ' + f.name + '.');
  };

  fileReader.readAsArrayBuffer(f);
}

// public interface
return {
  init : function(canvas)
  {
    Module._initialize_GL(1040, 1040);
    Main.resize();
  },

  resize : function()
  {
    // reset zoom
    var canvas = document.getElementById('canvas');
    var width = window.innerWidth;
    var height = window.outerHeight;
    Main.scaleCanvas(canvas, width, height);
    Main.alignInvisibleClick(canvas);
  },

  toggleFullscreen : function()
  {
    toggleFullscreen().then(Main.resize);
  },

  scaleCanvas : function(canvas, width, height)
  {
    var dim = width;
    if (height < dim) {
      dim = height;
    }
    console.log(dim);
    if (dim > 1040) {
      dim = 1040;
    }
    canvas.style.width = dim + "px";
    canvas.style.height = dim + "px";
  },

  alignInvisibleClick : function(canvas)
  {
     var cpos = canvas.getBoundingClientRect();
     var invisible_click = document.getElementById("invisible_click");
     invisible_click.style.width = canvas.style.width;
     invisible_click.style.height = canvas.style.height;
     invisible_click.style.top = cpos.top + "px";
     invisible_click.style.left = cpos.left + "px";
     invisible_click.style.zoom = canvas.style.zoom;
  },

  encode : function(filename, data)
  {
    console.log("encoding " + filename);
    var res = Module._encode(data.byteOffset, data.length, -1);
    console.log(res);
    Main.setTitle(filename);
    Main.setActive(true);
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

  clickNav : function()
  {
    document.getElementById("nav-button").focus();
  },

  blurNav : function()
  {
    document.getElementById("nav-button").blur();
    document.getElementById("nav-content").blur();
    document.getElementById("nav-find-file-link").blur();
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
    Main.blurNav();
  },

  nextFrame : function()
  {
    var start = performance.now();
    Module._render();
    var frameCount = Module._next_frame();

    var elapsed = performance.now() - start;
    var nextInterval = _interval>elapsed? _interval-elapsed : 0;
    setTimeout(Main.nextFrame, nextInterval);

    if (_showStats && frameCount) {
      _renderTime += elapsed;
      Main.setHTML( "status", elapsed + " : " + frameCount + " : " + Math.ceil(_renderTime/frameCount));
    }
  },

  setActive : function(active)
  {
    // hide cursor when there's a barcode active
    var invisi = document.getElementById("invisible_click");
    invisi.classList.remove("active");
    invisi.classList.add("active");
  },

  setMode : function(mode_str)
  {
    var is_4c = (mode_str == "4C");
    Module._configure(2, 255, 255, is_4c);

    var nav = document.getElementById("nav-container");
    if (is_4c) {
      nav.classList.remove("mode-b");
      nav.classList.add("mode-4c");
    } else if (mode_str == "B") {
      nav.classList.add("mode-b");
      nav.classList.remove("mode-4c");
    } else {
      nav.classList.remove("mode-b");
      nav.classList.remove("mode-4c");
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

window.addEventListener('keydown', function(e) {
  e = e || event;
  if (e.target instanceof HTMLBodyElement) {
    if (e.key == 'Enter' || e.keyCode == 13 ||
        e.key == 'Tab' || e.keyCode == 9 ||
        e.key == 'Space' || e.keyCode == 32
    ) {
      Main.clickNav();
      e.preventDefault();
    }
  }
  else {
    if (e.key == 'Escape' || e.keyCode == 27 ||
        e.key == 'Backspace' || e.keyCode == 8 ||
        e.key == 'End' || e.keyCode == 35 ||
        e.key == 'Home' || e.keyCode == 36
    ) {
      Main.blurNav();
    }
    else if (e.key == 'Tab' || e.keyCode == 9 ||
             e.key == 'ArrowDown' || e.keyCode == 40
    ) {
      var nav = document.getElementById('nav-button');
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      if (nav.classList.contains('attention')) {
        nav.classList.remove('attention');
        links[0].classList.add('attention');
        return;
      }
      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          var next = i+1 == links.length? nav : links[i+1];
          links[i].classList.remove('attention');
          next.classList.add('attention');
          break;
        }
      }
    }
    else if (e.key == 'ArrowUp' || e.keyCode == 38)
    {
      var nav = document.getElementById('nav-button');
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      if (nav.classList.contains('attention')) {
        nav.classList.remove('attention');
        links[links.length-1].classList.add('attention');
        return;
      }

      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          var next = i == 0? nav : links[i-1];
          links[i].classList.remove('attention');
          next.classList.add('attention');
          break;
        }
      }
    }
    else if (e.key == 'Enter' || e.keyCode == 13 ||
             e.key == ' ' || e.keyCode == 32
    ) {
      var nav = document.getElementById('nav-button');
      if (nav.classList.contains('attention')) {
        Main.blurNav();
        return;
      }
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          links[i].click();
        }
      }
    }
  }
}, true);

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
