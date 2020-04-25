[libcorrect](https://github.com/quiet/libcorrect)
===========
[![OSX/Linux Build Status](https://travis-ci.org/quiet/libcorrect.svg?branch=master)](https://travis-ci.org/quiet/libcorrect)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/i3e84jmj00fa5my8/branch/master?svg=true)](https://ci.appveyor.com/project/brian-armstrong/libcorrect/branch/master)

libcorrect is a library for Forward Error Correction. By using libcorrect, you can encode extra redundancy into a packet of data and then send it across a lossy channel. When the packet is received, it can be decoded to recover the original, pre-encoded data.

libcorrect accomplishes this task with two algorithms, [Convolutional codes](https://en.wikipedia.org/wiki/Convolutional_code) and [Reed-Solomon](https://en.wikipedia.org/wiki/Reed%E2%80%93Solomon_error_correction). Convolutional codes are robust to a constant background noise, while Reed-Solomon error correction is effective at dealing with noise that occurs in bursts. These algorithms have played an important role in [telecommunications](https://en.wikipedia.org/wiki/Error_detection_and_correction#Deep-space_telecommunications). libcorrect uses a [Viterbi algorithm](https://en.wikipedia.org/wiki/Viterbi_algorithm) decoder to decode convolutional codes.

libcorrect is a performant, BSD-licensed library. It is also the author's hope that this library's contents could help others learn how its algorithms work.

Design goals
-----------

1. libcorrect should be a drop-in, BSD-licensed substitute for [libfec](http://www.ka9q.net/code/fec/), which offers similar functionality under the LGPL-license. Although libfec is a fantastic library, the state of LGPL-licensed libraries on mobile devices is somewhat uncertain. For this reason, libcorrect is a completely new approach under the BSD license which supports the same algorithms as libfec. Additionally, libcorrect can be built with a compatibility layer so that libcorrect can be linked in place of libfec.

    Achieving this goal gives [libquiet](https://github.com/quiet/quiet) a fully BSD-/MIT-licensed set of dependencies, which gives libquiet more flexibility in mobile applications.

2. libcorrect should make it easier to investigate how forward error correction works. To accomplish this, libcorrect provides tools to test the fitness of convolutional codes and their polynomials. Additionally, libcorrect should be written in a way that leads to easy understanding of these powerful algorithms. This library's roadmap includes more documentation on how these algorithms work and how to increase their computational performance.

3. libcorrect should explore further into error correction. This goal would help libquiet operate in noisier situations. One approach might be to use parts of libcorrect's Viterbi Algorithm decoder to create a [Turbo code](https://en.wikipedia.org/wiki/Turbo_code) decoder, although this is just an idea and may turn out to be prohibitively difficult.

Build
-----------
libcorrect uses CMake, which allows for out-of-source builds. To get started, make sure that you have CMake installed, and then, from libcorrect's source directory, run `mkdir build && cd build && cmake .. && make && make install`. Additionally, if you would like the libfec compatibility layer, you can run `make shim && make install`, though do be cautioned that this can overwrite an existing installation of libfec.

If you are on a host which has `<x86intrin.h>` available, then libcorrect will automatically build its SSE version as well. The SSE headers are provided under `<correct-sse.h>`. For now, it is on the caller of this code to ensure that SSE is available and can be used. libcorrect requires SSE functions up to and including SSE4.

If you have any questions or problems with libcorrect, do not hesitate to open an issue.

-----------
I'd like to thank Ryan Hitchman and Josh Gao for all of their help and rubber ducking.

A huge thank you goes to [Lucas Teske](https://github.com/racerxdl) for finding all the ways that libcorrect was broken on Windows and to [Denis Golovan](https://github.com/MageSlayer) for finding an error in the returned length of the convolutional code decoder.




