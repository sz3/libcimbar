### [简介](https://github.com/sz3/cimbar) | [关于](https://github.com/sz3/cimbar/blob/master/ABOUT.md) | [CFC](https://github.com/sz3/cfc) | LIBCIMBAR
### [详细内容](DETAILS.md) | [演示](PERFORMANCE.md) | [下一步](TODO.md)

## libcimbar: 彩色图形矩阵条形码

一种用于小型数据传输的实验性条形码。

解码速度在 850Kbit/s 左右（约106 KB/s），只需一个电脑显示器和手机摄像头！

<p align="center">
<img src="https://github.com/sz3/cimbar-samples/blob/v0.6/b/4cecc30f.png" width="70%" title="A non-animated mode-B cimbar code" >
</p>

## 为什么？

编码器可以将条形码输出到计算机或手机屏幕：
* 网页端：https://cimbar.org

也可以是一个使用手机摄像头读取条形码的手机APP：
* 安卓APP：https://github.com/sz3/cfc

APP并未使用互联网/蓝牙/NFC等，所有数据都是通过摄像头传输的。

## 原理

`cimbar`是一种高密度的二维条形码格式。数据存储在彩色图块网格中——根据选择哪个图块和选择哪种颜色来绘制图块，对信息进行编码。对数据应用Reed-Solomon纠错，预计错误率在1%左右。

`libcimbar`，这是个优化版本，包括一个基于喷泉代码（`wirehair`）和zstd压缩的简单文件编码协议。最高33MB（压缩后）的文件以一系列 `cimbar` 代码编码，输出为图像或视频等。一旦成功解码了足够多帧，该文件将被重建和解压缩。即使图像接收出错，或者有些图像已损坏或丢失，也不会有太大问题。

## 平台

该代码是用C++编写的，并在 amd64+linux、arm64+android（仅限解码器）和 emscripten+WASM（仅限编码器）上开发/测试。在其他平台上应该也可以工作。

至关重要的是，由于编码器编译为 asmjs 和 wasm ，它可以在任何具有现代网络浏览器的设备上运行。对于离线使用，您可以将 `cimbar.org` 安装为渐进式web应用程序，或[下载最新版本](https://github.com/sz3/libcimbar/releases/latest)。至于 `cimbar_js.html` ，将其保存在本地，然后在浏览器中打开。

## 依赖库

[OpenCV](https://opencv.org/) 和 [GLFW](https://github.com/glfw/glfw) (+ OpenGL ES headers) 必须在构建前安装，所有其他依赖项都包含在源代码树中。

* opencv - https://opencv.org/ (`libopencv-dev`)
* GLFW - https://github.com/glfw/glfw (`libglfw3-dev`)
* GLES3/gl3.h - `libgles2-mesa-dev`
* base - https://github.com/r-lyeh-archived/base
* catch2 - https://github.com/catchorg/Catch2
* concurrentqueue - https://github.com/cameron314/concurrentqueue
* cxxopts - https://github.com/jarro2783/cxxopts （用于命令行工具）
* fmt - https://github.com/fmtlib/fmt
* intx - https://github.com/chfast/intx
* libcorrect - https://github.com/quiet/libcorrect
* libpopcnt - https://github.com/kimwalisch/libpopcnt
* PicoSHA2 - https://github.com/okdshin/PicoSHA2 （用于测试）
* stb_image - https://github.com/nothings/stb （用于加载嵌入图像）
* wirehair - https://github.com/catid/wirehair
* zstd - https://github.com/facebook/zstd

## 构建

1. 在 Ubuntu/Debian 上安装 opencv 和 GLFW 。如下：
```
sudo apt install libopencv-dev libglfw3-dev libgles2-mesa-dev
```

2. 运行 cmake + make
```
cmake .
make -j7
make install
```

默认情况下，`libcimbar` 将尝试在 `./dist/bin/` 下安装构建产品。

要构建 `cimbar.js`（供`cimbar.org`使用)，请参阅 [WASM](WASM.md)。

## 使用

编码：
* 输入文件太大可能导致硬盘中生成过多图片！

```
./cimbar --encode -i inputfile.txt -o outputprefix
```

解码（释放文件到输出目录）：
```
./cimbar outputprefix*.png -o /tmp
```

从`stdin`解码一系列编码图像：
```
echo outputprefix*.png | ./cimbar -o /tmp
```

对窗口进行编码并设置动画：
```
./cimbar_send inputfile.pdf
```

您还可以使用 [cimbar.org](https://cimbar.org) 对文件进行编码，或最新的 [release](https://github.com/sz3/libcimbar/releases/latest).

## 演示

[PERFORMANCE](PERFORMANCE.md)

## 详细信息

[DETAILS](DETAILS.md)

## 下一步

[TODO](TODO.md)

## 灵感来源

* https://github.com/JohannesBuchner/imagehash/
* https://github.com/divan/txqr
* https://en.wikipedia.org/wiki/High_Capacity_Color_Barcode

## 想了解更多？

### [cimbar](https://github.com/sz3/cimbar) | [关于](https://github.com/sz3/cimbar/blob/master/ABOUT.md)
