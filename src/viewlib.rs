extern crate clap;
extern crate time;
extern crate libc;

use std::ffi::{CString};
use std::os::raw::c_char;
use std::f64;
use libc::{c_double, c_float, FILE};
use std::slice;
use std::fs::File;
use std::io::stdout;

// Link in the C lib via FFI
#[link(name = "view3d", kind = "static")]
extern "C" {
    pub fn parseInPath(infile: *const c_char) -> RawInData;
    pub fn calculateVFs(rawInData: RawInData) -> VFResultsC;
    pub fn processPaths2d(infile: *const c_char, outfile: *const c_char);
    pub fn printVFs(format: i32, file: *mut FILE, results: VFResultsC);
    pub fn freeVFResultsC(results: VFResultsC);
}


pub fn process_paths(infile: String, outfile: String) -> VFResults {
    // Convert these arguments to C strings to use in FFI
    let infile_c = CString::new(infile).expect("CString::new failed");
    let outfile_c = CString::new(outfile).expect("CString::new failed");
    unsafe {

        let in_data = parseInPath(infile_c.as_ptr());
        let vf_res = calculateVFs(in_data);
        println!("{:?}", vf_res);
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
    pub didemit: i32,
    pub area: *const c_float,
    pub emit: *const c_float,
    pub values: *const c_double,
    pub AF: *const *const c_double,
}

// #[derive(Debug)]
#[repr(C)]
pub struct RawInData {
  pub opts: RawInOptions,
  pub nAllSrf: i32,
  pub nRadSrf: i32,
  pub nObstrSrf: i32,
  pub nVertices: i32,
  pub vertices: [Vec3; 256],
  pub surfaces: [RawSurf; 256],
}

#[derive(Debug)]
#[repr(C)]
pub struct RawInOptions {
  pub title: *const c_char,
  pub epsAdap: c_float,
  pub maxRecursALI: i32,
  pub minRecursion: i32,
  pub maxRecursion: i32,
  pub enclosure: i32,
  pub emittances: i32,
  pub row: i32,
  pub col: i32,
  pub prjReverse: i32,
}

#[derive(Debug)]
#[repr(C)]
pub struct RawSurf {
  pub nr: i32,
  pub nv: i32,
  pub type_: i32,
  pub base: i32,
  pub cmbn: i32,
  pub emit: c_float,
  pub vertexIndices: [i32; 4],
  pub name: [c_char; 16],
//   char name[NAMELEN];
}

#[derive(Debug)]
#[repr(C)]
pub  struct Vec3 {
  x: c_double,
  y: c_double,
  z: c_double,
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
