extern crate viewlib;

use clap::{Arg, App};
use std::ffi::{CString};

use crate::viewlib::{processPaths2d};

fn main() {
    let matches = App::new("View2d")
            .version("3.5")
            .author("Jake O'Shannessy <joshannessy@gmail.com>")
            .about("Compute view factors for a 2D geometry.")
            .arg(Arg::with_name("INFILE")
                .help("The input file")
                .value_name("INFILE"))
            .arg(Arg::with_name("OUTFILE")
                .help("The output file")
                .value_name("OUTFILE"))
            .get_matches();

    let infile = matches.value_of("INFILE").expect("Input file not provided");
    // If no outfile is specified, pass an empty string and the library will
    // understand that it means direct to stdout
    let outfile = matches.value_of("OUTFILE").unwrap_or("");
    // Convert these arguments to C strings to use in FFI
    let infile_c = CString::new(infile).expect("CString::new failed");
    let outfile_c = CString::new(outfile).expect("CString::new failed");

    unsafe {
        processPaths2d(infile_c.as_ptr(), outfile_c.as_ptr());
    }
}
