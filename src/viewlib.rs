extern crate clap;
extern crate time;
extern crate libc;

use std::ffi::{CString};
use std::os::raw::c_char;
use std::f64;
use libc::{c_double, c_float};
use std::slice;

// Link in the C lib via FFI
#[link(name = "view3d", kind = "static")]
extern "C" {
    fn processPaths(infile: *const c_char, outfile: *const c_char) -> VFResultsC;
}

#[link(name = "view3d", kind = "static")]
extern "C" {
    pub fn processPaths2d(infile: *const c_char, outfile: *const c_char);
}


pub fn process_paths(infile: String, outfile: String) -> VFResults {
    // Convert these arguments to C strings to use in FFI
    let infile_c = CString::new(infile).expect("CString::new failed");
    let outfile_c = CString::new(outfile).expect("CString::new failed");
    unsafe {
        let vf_res = processPaths(infile_c.as_ptr(), outfile_c.as_ptr());

        // Convert the view factor values to a vector
        let af_arr_ptr = vf_res.values;
        assert!(!af_arr_ptr.is_null());
        let res: &[f64] = slice::from_raw_parts(af_arr_ptr, (vf_res.n_surfs*vf_res.n_surfs) as usize);
        let res2 = res.clone();
        let vec = res2.to_vec();

        // Convert the area values to a vector
        let area_ptr = vf_res.area;
        assert!(!area_ptr.is_null());
        let area_raw: &[f32] = slice::from_raw_parts(area_ptr, vf_res.n_surfs as usize);
        let area_raw2 = area_raw.clone();
        let areas = area_raw2.to_vec();

        // Convert the emissivity values to a vector
        let emit_ptr = vf_res.emit;
        assert!(!emit_ptr.is_null());
        let emit_raw: &[f32] = slice::from_raw_parts(emit_ptr, vf_res.n_surfs as usize);
        let emit_raw2 = emit_raw.clone();
        let emit = emit_raw2.to_vec();

        // Convert the enclosure flag to a bool
        let encl = if vf_res.encl == 0 { false } else { true };

        VFResults {
            n_surfs: vf_res.n_surfs as u32,
            encl,
            areas,
            emit,
            values: vec,
        }
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct VFResultsC {
    pub n_surfs: i32,
    pub encl: i32,
    pub area: *const c_float,
    pub emit: *const c_float,
    pub values: *const c_double,
}

#[derive(Debug)]
pub struct VFResults {
    pub n_surfs: u32,
    pub encl: bool,
    pub areas: Vec<f32>,
    pub emit: Vec<f32>,
    pub values: Vec<f64>,
}

impl VFResults {
    pub fn vf(&self, a: usize, b: usize) -> f64 {
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

pub fn print_vf_results(results: &VFResults) {
    println!("encl: {}", results.encl);
    // Print column numbering
    print!("      ");
    for i in 1..=(results.n_surfs as usize) {
        print!("{:^8} ", i);
    }
    println!("");
    // Print areas
    print!("area: ");
    for area in results.areas.iter() {
        print!("{:.*} ", 6, area);
    }
    println!("");
    // Print emissivities
    print!("emit: ");
    for emit in results.emit.iter() {
        print!("{:.*} ", 6, emit);
    }
    println!("");
    // Print separator
    print!("      ");
    for _ in 1..=(results.n_surfs as usize) {
        print!("---------");
    }
    println!("");
    // Print view factors
    for (i, value) in results.values.iter().enumerate() {
        if i % (results.n_surfs as usize) == 0 {
            print!("{:4}: ", (i /(results.n_surfs as usize)) + 1);
        }
        print!("{:.*}", 6, value);
        if (i + 1) % (results.n_surfs as usize) == 0 {
            println!("");
        } else {
            print!(" ");
        }
    }
}

pub fn analytic_1(width: f64, height: f64) -> f64 {
    let w = width/height;
    let x = (1_f64+w.powi(2)).sqrt();
    let y = (w/x).atan()*x-(w).atan();
    let f12 = (1_f64/(f64::consts::PI*w.powi(2)))*((x.powi(4)/(1_f64+2_f64*w.powi(2))).ln()+4_f64*w*y);
    f12
}
