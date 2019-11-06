use clap::{App, Arg};

use std::error::Error;
use std::fs::File;
use std::path::Path;
use view3d::{print_vf_results, process_path};

fn main() {
    let matches = App::new("View3d")
        .version("3.5")
        .author("Jake O'Shannessy <joshannessy@gmail.com>")
        .about("Compute view factors for a 3D geometry..")
        .arg(
            Arg::with_name("INPUT-FILE")
                .help("The input file")
                .value_name("INPUT-FILE")
                .required(true),
        )
        .arg(
            Arg::with_name("OUTPUT-FILE")
                .help("The output file")
                .value_name("OUTPUT-FILE"),
        )
        .get_matches();

    let infile = matches.value_of("INPUT-FILE").expect("Input file not provided");
    let vf_results = process_path(infile.to_string());
    // TODO: redirect to file if requested
    let outfile = matches.value_of("OUTPUT-FILE").unwrap_or("");
    let res: std::io::Result<()> = if outfile == "" {
        let stdout = std::io::stdout();
        let mut handle = stdout.lock();
        print_vf_results(&mut handle, &vf_results)
    } else {
        let outpath = Path::new(outfile);
        let mut file = match File::create(&outpath) {
            Err(why) => panic!(
                "couldn't create {}: {}",
                outpath.display(),
                why.description()
            ),
            Ok(file) => file,
        };
        // Write the data to the file
        match print_vf_results(&mut file, &vf_results) {
            Err(e) => Err(e),
            // Ensure all data is completely written to disk
            Ok(_) => file.sync_all(),
        }
    };
    println!("1->8: {:?}", vf_results.vf(1, 8));
    println!("8->1: {:?}", vf_results.vf(8, 1));
    match res {
        Ok(_) => (),
        Err(e) => {
            println!("filed to write results");
            println!("{}", e);
            panic!(e);
        }
    }
}

#[cfg(test)]
mod integration {
    use assert_cmd;
    use assert_cmd::prelude::*;
    use std::process::Command;

    /// We currently ignore this test as the output formats are not the same
    #[test]
    #[ignore]
    fn calling_deploy_example() {
        let expected = std::fs::read_to_string("test/3d/test.expected.out")
            .expect("something went wrong reading the expected output file");

        let result = Command::cargo_bin("view3d")
            .unwrap()
            .args(&["test/3d/test.vs3"])
            .output()
            .unwrap();
        assert_eq!(
            expected,
            String::from_utf8(result.stdout).expect("Output invalid utf8")
        );
    }
}
