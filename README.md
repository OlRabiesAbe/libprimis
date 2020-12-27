## Libprimis

Check out the project timeline at `doc/timeline.md`!

#### A True Open Source Engine

Libprimis is a game engine, based on Tesseract and the Cube 2 family of programs.
Unlike the Cube/Cube 2/Tesseract games, which featured tightly integrated rendering
and game code, however, Libprimis is *just an engine*.

Libprimis allows game makers to build unique and distinct games by providing an
API for its octree-based geometry system. While this is not particularly exceptional
for a game engine (which obviously have to have an API of some sort), Libprimis is
the first adaptation of the Cube2/Tesseract capable of providing the power of the
octree without dealing with confusing and poorly defined boundaries between game
and rendering code.

With many modern features, including realtime deferred shading, volumetric lighting, and
tone mapping support, Libprimis' core is fast, capable, and modern, and fully open sourced.
All this combines to make an engine that allows for an unprecedented ability to manipulate
a vibrant and dynamic world using simple, accessible semantics.

#### Key Features

Libprimis' Tesseract base provides a bunch of rendering features such as:

* deferred shading
* omnidirectional point lights using cubemap shadowmaps
* perspective projection spotlight shadowmaps
* orthographic projection sunlight using cascaded shadowmaps
* HDR rendering with tonemapping and bloom
* real-time diffuse global illumination for sunlight (radiance hints)
* volumetric lighting
* screen-space ambient occlusion
* screen-space reflections and refractions for water and glass
* screen-space refractive alpha cubes
* deferred MSAA, subpixel morphological anti-aliasing (SMAA 1x, T2x, S2x, and 4x), FXAA, and temporal AA
* support for OpenGL 4.0+ contexts
* support for Linux-based operating systems

**NOTE:** Libprimis currently lacks Windows install semantics, though progress is currently being made.
Check back later for Windows support.

For documentation on the engine, see `doc/engine.md`.


#### Quick Linux Install Instructions

To get the game, `git` is required. Using `git`, get the repository and its submodules with

`git clone https://github.com/project-imprimis/libprimis.git --recurse-submodules`

The `libprimis` folder will now be visible in the current directory.

To compile the game, use `make -C src -jN` from the directory in which this file is located.
Set N to the number of threads to compile with. For example, for a quad-core processor, set -j4.

(to reach this directory use `cd libprimis`)

This game requires `libsdl2`, `libsdl2-image`, `libsdl2-mixer`, and drivers for OpenGL (usually already installed).
To compile the game, the development versions of the libraries are required
(on distros that seperate standard and dev packages).

Once the library has been compiled, it should be placed the standard shared library folder
(usually `/usr/lib/`) where it can be linked to. Alternatively, use `make -Csrc install` to
automatically compile and install the library to `/usr/lib/`.

To build a game on libprimis, you will then need to get the required headers (located in a separate repository)
and build your game against these headers and the shared library.

#### Join Us

Libprimis is an open source project created by volunteers who work on the game as
a hobby, and we'd love for it to be your hobby too! The Libprimis project tries
to be well documented and transparent in its decision making so as to make
outside participation fruitful. If you'd like to express your opinions on the
engine's decision, modify the engine, participate on the engine code, or just say
hello to the developers, that's great! We have a Discord server where you may
interact with us at https://discord.gg/WVFjtzA.

To facilitate getting started working on Libprimis, there are several issues posted
on the "issues" board. Whether you're a longtime open source contributor or you
need to create a GitHub account to start participating, feel free to use issues
labeled as "good first issue" to ask whatever questions you have about Git semantics
or quirks about our specific codebase in order to get comfortable!
