extern crate clap;
extern crate time;
extern crate libc;

use clap::{Arg, App};

mod viewlib;
use viewlib::*;

fn main() {
    let matches = App::new("View3d")
            .version("3.5")
            .author("Jake O'Shannessy <joshannessy@gmail.com>")
            .about("Compute view factors for a 3D geometry..")
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

    let vf_results = process_path(infile.to_string());
    print_vf_results(&vf_results);
    println!("1->8: {:}", vf_results.vf(1,8));
    println!("8->1: {:}", vf_results.vf(8,1));
}
