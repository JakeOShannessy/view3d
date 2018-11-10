# View3D

This is an expanded repo based on the original View3D by George Walton and
John Pye. The README for the original project can be found at README.txt.

This repo consists of 4 parts:

- A C library.
- C executables using the library.
- A Rust binding to the library.
- Rust executables using the library.

## Building C Library and Executables

A Makefile is provided as an example, but the preferred method of building is
with CMake.

### GCC, Bash, etc.

```sh
# Make a build directory
mkdir build
cd build
# Generate the makefile using cmake
cmake ..
# Run the makefile
make
```

### MSVC and CMD

Scripts are provided for convenience. These scripts will find and set up the
Visual Studio compiler where it is able. To build use:

```cmd
build
```

And to clean the build files use:

```cmd
clean
```

## Building Rust Library and Executables

To build the Rust library and bindings use:

```sh
cargo build
```

Cargo will handle the building of the C library for linking on most platforms.

## Differences from the Original

This repo does not attempt to main the 2D/3D viewer programs using QT and
Coin3D.
