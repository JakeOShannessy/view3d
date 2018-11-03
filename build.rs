use std::process::Command;
use std::env;
use std::path::Path;

// fn main() {
//     let out_dir = env::var("OUT_DIR").unwrap();

//     // note that there are a number of downsides to this approach, the comments
//     // below detail how to improve the portability of these commands.
//     Command::new("make")
//                     // .args(&["make"])
//                     //    .arg(&format!("{}/hello.o", out_dir))
//                        .status().unwrap();
// }

extern crate cc;

fn main() {
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
        // .file("v2main.c")
        .file("v3main.c")
        .include(".")
        .define("LIBONLY", Some("1"))
        .compile("view3d");
}

