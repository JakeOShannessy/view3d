# View3D

This is an expanded repo based on the original View3D by George Walton and
John Pye. The README for the original project can be found at README.txt.

This repo consists of 4 parts:

- A C library.
- C executables using the library.
- A Rust binding to the library.
- Rust executables using the library.

## Building

To build the C library, use:

```sh
make lib
```

And to make the C executables use:

```sh
make all
```

To build the Rust library and bindings use:

```sh
cargo build
```

Cargo will handle the building of the C library for linking.

## Differences from the Original

This repo does not attempt to main the 2D/3D viewer programs using QT and
Coin3D.
