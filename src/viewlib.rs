extern crate clap;
extern crate time;
extern crate libc;

use clap::{Arg, App};
use std::ffi::{CString};
use std::os::raw::c_char;
use std::f64;
use libc::{uint32_t, size_t, c_double};
use std::slice;

// Link in the C lib via FFI
#[link(name = "view3d", kind = "static")]
extern "C" {
    fn processPaths(infile: *const c_char, outfile: *const c_char) -> VFResultsC;
}

pub fn process_paths(infile: String, outfile: String) -> VFResults {
    // Convert these arguments to C strings to use in FFI
    let infile_c = CString::new(infile).expect("CString::new failed");
    let outfile_c = CString::new(outfile).expect("CString::new failed");
    unsafe {
        let vf_res = processPaths(infile_c.as_ptr(), outfile_c.as_ptr());
        let af_arr_ptr = vf_res.values;

        assert!(!af_arr_ptr.is_null());
        let res: &[f64] = slice::from_raw_parts(af_arr_ptr, vf_res.n_values as usize);
        let res2 = res.clone();
        let vec = res2.to_vec();
        VFResults {
            n_surfs:(vf_res.n_values as f32).sqrt().round() as u32,
            values: vec,
        }
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct VFResultsC {
    pub n_values: i32,
    pub values: *const c_double,
}

pub fn print_vf_results(results: &VFResults) {
    print!("    ");
    for i in 1..=(results.n_surfs as usize) {
        print!("{:^8} ", i);
    }
    println!("");
    for (i, value) in results.values.iter().enumerate() {
        if i % (results.n_surfs as usize) == 0 {
            print!("{:2}: ", (i /(results.n_surfs as usize)) + 1);
        }
        print!("{:.*}", 6, value);
        if (i + 1) % (results.n_surfs as usize) == 0 {
            println!("");
        } else {
            print!(" ");
        }
    }
}

#[derive(Debug)]
pub struct VFResults {
    pub n_surfs: u32,
    pub values: Vec<f64>,
}

impl VFResults {
    fn vf(&self, a: usize, b: usize) -> f64 {
        self.values[(a-1)*(self.n_surfs as usize)+(b-1)]
    }
}
// pub struct VFResultsC {
//     program: CString,
//     program_version: CString,
//     format: i32,
//     encl: i32,
//     didemit: i32,
//     nSrf: i32,
//     float *area,
//     float *emit,
//     double **AF,
// }

pub fn analytic_1(width: f64, height: f64) -> f64 {
    let w = width/height;
    let x = (1_f64+w.powi(2)).sqrt();
    let y = (w/x).atan()*x-(w).atan();
    let f12 = (1_f64/(f64::consts::PI*w.powi(2)))*((x.powi(4)/(1_f64+2_f64*w.powi(2))).ln()+4_f64*w*y);
    f12
}
