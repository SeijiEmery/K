
"K": new game frameworkey thing I'm working on in my spare time (C++ / D).

Work in Progress.

Currently I'm working on building a multithreaded application model on top of GLFW,
which will form the foundation of everything else.

Project structure:
    src/app/include         public application 'Client' interface (mostly facades)
    src/app/src             private application implementation + internals; WIP.
    src/app/tests           unit tests (TBD)

    src/util/include        various utilities + shared code (CommandBuffer, etc)
    src/util/tests          unit tests (TBD)

    ext/                    external libraries

    demos/                  K feature tests
    demos/window_test/src   1st window / event test (WIP)


Note: this is (effectively) the 4th iteration of GLSandbox. I do rewrites when I need
to get down to the bare essentials + rewrite systems w/out breaking the reference 
implementations + demos I built before. And usually this is when I switch build
systems (GLSandbox = Xcode / OSX only, GSB / SB = dub based, K = Cmake).

I could just try to implement this on top of GSB (and eg. switch the build system
to cmake), but sometimes it's much cleaner to start from scratch, and I won't be
sharing any source code initially.


FEATURE GOALS:
– Robust, high-level wrappers built around GLFW3 + OpenGL.
– Many threads; internal communication via CommandQueues (CommandBuffer)
– Better library integration; less writing stuff from scratch. Planning to include:
    – Boost
    – glm
    – OpenECS   (https://github.com/Gronis/OpenEcs)
    – OpenMesh
    – nanovg / nanogui
    – box2d
    – stb libraries (ofc)
    – googletest (unit tests) + haiyai (benchmarking)
    – expected  (https://github.com/ptal/expected)
    – etc.
– XML-based asset + scene format (have specs; planning to implement via SAX parser;
  may use boost spirit and write my own b/c most suck and I'm using a restricted
  subset w/ focus on tag attributes, not deeply nested tags to store data -- human
  readability is key people...)
– More data-oriented (in general). Gamepads + key bindings should be user configurable,
  not hardcoded, etc.
