var Main = function() {

var _interval = 66;
var _pause = 0;

var _showStats = false;
var _counter = 0;
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
    Main.check_GL_enabled(canvas);
  },

  check_GL_enabled : function(canvas)
  {
    if (canvas.getContext("2d")) {
       var elem = document.getElementById('dragdrop');
       elem.classList.add("error");
    }
  },

  resize : function()
  {
    // reset zoom
    var canvas = document.getElementById('canvas');
    var width = window.innerWidth - 10;
    var height = window.innerHeight - 10;
    Main.scaleCanvas(canvas, width, height);
    Main.alignInvisibleClick(canvas);
  },

  toggleFullscreen : function()
  {
    toggleFullscreen().then(Main.resize);
    Main.togglePause(true);
  },

  togglePause : function(pause)
  {
    // pause is a cooldown. We pause to help autofocus, but we don't want to do it forever...
    if (pause === undefined) {
       pause = !Main.isPaused();
    }
    _pause = pause? 15 : 0;
  },

  isPaused : function()
  {
     return _pause > 0;
  },

  scaleCanvas : function(canvas, width, height)
  {
    var dim = width;
    if (height < dim) {
      dim = height;
    }
    console.log(dim + "x" + dim);
    canvas.style.width = dim + "px";
    canvas.style.height = dim + "px";
  },

  alignInvisibleClick : function(canvas)
  {
     canvas = canvas || document.getElementById('canvas');
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

  blurNav : function(pause)
  {
    if (pause === undefined) {
       pause = true;
    }
    document.getElementById("nav-button").blur();
    document.getElementById("nav-content").blur();
    document.getElementById("nav-find-file-link").blur();
    Main.togglePause(pause);
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
    Main.blurNav(false);
  },

  nextFrame : function()
  {
    _counter += 1;
    if (_pause > 0) {
       _pause -= 1;
    }
    var start = performance.now();
    if (!Main.isPaused()) {
       Module._render();
       var frameCount = Module._next_frame();
    }

    var elapsed = performance.now() - start;
    var nextInterval = _interval>elapsed? _interval-elapsed : 0;
    setTimeout(Main.nextFrame, nextInterval);

    if (_showStats && frameCount) {
      _renderTime += elapsed;
      Main.setHTML( "status", elapsed + " : " + frameCount + " : " + Math.ceil(_renderTime/frameCount));
    }
    if ( !(_counter & 31) ) {
       Main.resize();
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
    else if (e.key == 'Backspace' || e.keyCode == 8) {
      Main.togglePause(true);
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

window.addEventListener("touchstart", function(e) {
  e = e || event;
  Main.togglePause(true);
}, false);

window.addEventListener("touchend", function(e) {
  e = e || event;
  Main.togglePause(false);
}, false);

window.addEventListener("touchcancel", function(e) {
  e = e || event;
  Main.togglePause(false);
}, false);

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
