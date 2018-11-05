use std::process::Command;
use std::env;
use std::path::Path;

extern crate cc;

fn main() {

    Command::new("make")
        .args(&["config.h"])
        .status().unwrap();

    cc::Build::new()
        .file("ctrans.c")
        .file("heap.c")
        .file("polygn.c")
        .file("savevf.c")
        .file("viewobs.c")
        .file("viewunob.c")
        .file("getdat.c")
        .file("readvf.c")
        .file("readvs.c")
        .file("test3d.c")
        .file("view3d.c")
        .file("viewpp.c")
        .file("common.c")
        .file("view2d.c")
        .file("test2d.c")
        .file("misc.c")
        .file("v2lib.c")
        .file("v3lib.c")
        .include(".")
        .compile("view3d");
}
