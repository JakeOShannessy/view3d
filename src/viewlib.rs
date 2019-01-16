extern crate clap;
extern crate time;
extern crate libc;

use std::ffi::{CString, CStr};
use std::os::raw::c_char;
use std::f64;
use libc::{c_double, c_float, FILE};
use std::slice;
use std::io::{Write};
#[cfg(test)]
use quickcheck::*;

// Link in the C lib via FFI
#[link(name = "view3d", kind = "static")]
extern "C" {
    pub fn parseInPath(infile: *const c_char) -> RawInData;
    pub fn calculateVFs(rawInData: RawInData) -> VFResultsC;
    pub fn processPaths2d(infile: *const c_char, outfile: *const c_char);
    pub fn printVFs(format: i32, file: *mut FILE, results: VFResultsC);
    pub fn freeVFResultsC(results: VFResultsC);
}

pub fn process_path(infile: String) -> VFResults {
    // Convert these arguments to C strings to use in FFI
    let infile_c = CString::new(infile).expect("CString::new failed");
    unsafe {

        let in_data = parseInPath(infile_c.as_ptr());
        let vf_res = calculateVFs(in_data);
        // From here we copy the data into a Rust native struct. This is more
        // expensive then simply abstracting over the underlying C type, but it
        // requires us to implement a lot less and is less error prone.
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
        let final_res = VFResults {
            n_surfs: vf_res.n_surfs as u32,
            encl,
            areas,
            emit,
            values: vec,
        };
        freeVFResultsC(vf_res);
        final_res
    }
}

pub fn process_v3d(in_data: InData) -> VFResults {
    unsafe {
        // Run the calculation. We convert the in_data into the C type before we do this.
        let vf_res = calculateVFs(in_data.into());
        // From here we copy the data into a Rust native struct. This is more
        // expensive then simply abstracting over the underlying C type, but it
        // requires us to implement a lot less and is less error prone.
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
        let final_res = VFResults {
            n_surfs: vf_res.n_surfs as u32,
            encl,
            areas,
            emit,
            values: vec,
        };
        freeVFResultsC(vf_res);
        final_res
    }
}

/// This is the type that the C library returns. This is only used temporarily,
/// and the data is copied into a native Rust struct for ease of implementation.
#[derive(Debug)]
#[repr(C)]
pub struct VFResultsC {
    pub n_surfs: i32,
    pub encl: i32,
    pub didemit: i32,
    pub area: *const c_float,
    pub emit: *const c_float,
    pub values: *const c_double,
    pub af: *const *const c_double,
    pub row: i32,
    pub n_srf0: i32,
}

#[repr(C)]
pub struct RawInData {
    pub opts: RawInOptions,
    pub n_all_srf: i32,
    pub n_rad_srf: i32,
    pub n_obstr_srf: i32,
    pub n_vertices: i32,
    pub vertices: [Vec3; 256],
    pub surfaces: [RawSurf; 256],
}

impl std::fmt::Debug for RawInData {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        let _props = write!(formatter, "RawInData {{
    opts: {:?},
    n_all_srf: {:?},
    n_rad_srf: {:?},
    n_obstr_srf: {:?},
    n_vertices: {:?},
    vertices: [Vec3; 256],
    surfaces: [RawSurf; 256],
}}", self.opts, self.n_all_srf, self.n_rad_srf, self.n_obstr_srf, self.n_vertices);
        let _verts : std::result::Result<(), std::fmt::Error> = self.vertices
            .into_iter()
            .map(|vertex| write!(formatter, "{:?}\n", vertex))
            .collect();
        let _surfs : std::result::Result<(), std::fmt::Error> = self.surfaces
            .into_iter()
            .map(|surface| write!(formatter, "{:?}\n", surface))
            .collect();
        Ok(())
    }
    
}

/// Input data using native Rust types
pub struct InData {
    pub opts: InOptions,
    pub n_all_srf: i32,
    pub n_rad_srf: i32,
    pub n_obstr_srf: i32,
    pub n_vertices: i32,
    pub vertices: Vec<Vec3>,
    pub surfaces: Vec<RawSurf>,
}

impl From<RawInData> for InData {
    fn from(raw: RawInData) -> Self {
        let mut vertices = raw.vertices.to_vec();
        vertices.truncate(raw.n_vertices as usize);
        let mut surfaces = raw.surfaces.to_vec();
        surfaces.truncate(raw.n_all_srf as usize);
        InData {
            opts: raw.opts.into(),
            n_all_srf: raw.n_all_srf,
            n_rad_srf: raw.n_rad_srf,
            n_obstr_srf: raw.n_obstr_srf,
            n_vertices: raw.n_vertices,
            vertices,
            surfaces,
        } 
    }
}

fn from_slice(bytes: &[u8]) -> [u8; 256] {
    let mut array = [0; 256];
    let bytes = &bytes[..array.len()]; // panics if not enough data
    array.copy_from_slice(bytes); 
    array
}



impl From<InData> for RawInData {
    fn from(input: InData) -> Self {
        let mut vertices: [Vec3; 256] = [Vec3 {x: 0_f64, y: 0_f64, z: 0_f64}; 256];
        let mut surfaces: [RawSurf; 256] = [RawSurf { nr: 0, nv: 0, type_: 0, base: 0, cmbn: 0, emit: 0_f32, vertex_indices: [0; 4], name: [0; 16]}; 256];
        for (i, vertex) in input.vertices.iter().enumerate() {
            vertices[i+1] = vertex.clone();
        }
        for (i, surface) in input.surfaces.iter().enumerate() {
            surfaces[i+1] = surface.clone();
        }
        // from_slice(input.vertices.as_slice());
        // let surfaces = from_slice(input.surfaces.as_slice());
        RawInData {
            opts: input.opts.into(),
            n_all_srf: input.n_all_srf,
            n_rad_srf: input.n_rad_srf,
            n_obstr_srf: input.n_obstr_srf,
            n_vertices: input.n_vertices,
            vertices,
            surfaces,
        } 
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct RawInOptions {
  pub title: *const c_char,
  pub eps_adap: c_float,
  pub max_recurs_ali: i32,
  pub min_recursion: i32,
  pub max_recursion: i32,
  pub enclosure: i32,
  pub emittances: i32,
  pub row: i32,
  pub col: i32,
  pub prj_reverse: i32,
}

#[derive(Debug)]
pub struct InOptions {
  pub title: String,
  pub eps_adap: f32,
  pub max_recurs_ali: i32,
  pub min_recursion: i32,
  pub max_recursion: i32,
  pub enclosure: bool,
  pub emittances: bool,
  pub row: i32,
  pub col: i32,
  pub prj_reverse: bool,
}

impl From<RawInOptions> for InOptions {
    fn from(raw: RawInOptions) -> Self {
        InOptions {
            title: unsafe {CStr::from_ptr(raw.title).to_string_lossy().into_owned()},
            eps_adap: raw.eps_adap,
            max_recurs_ali: raw.max_recurs_ali,
            min_recursion: raw.min_recursion,
            max_recursion: raw.max_recursion,
            enclosure: if raw.enclosure == 0 {false} else {true},
            emittances: if raw.emittances == 0 {false} else {true},
            row: raw.row,
            col: raw.col,
            prj_reverse: if raw.prj_reverse == 0 {false} else {true},
        } 
    }
}

impl From<InOptions> for RawInOptions {
    fn from(input: InOptions) -> Self {
        // let s = CString::new(input.title).expect("CString::new failed");
        // println!("s: {:?}", s);
        // println!("s.as_ptr(): {:?}", s.as_ptr());
        RawInOptions {
            title: CString::new(input.title).expect("CString::new failed").as_ptr(),
            eps_adap: input.eps_adap,
            max_recurs_ali: input.max_recurs_ali,
            min_recursion: input.min_recursion,
            max_recursion: input.max_recursion,
            enclosure: if input.enclosure {1} else {0},
            emittances: if input.emittances {1} else {0},
            row: input.row,
            col: input.col,
            prj_reverse: if input.prj_reverse {1} else {0},
        } 
    }
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct RawSurf {
  pub nr: i32,
  pub nv: i32,
  pub type_: i32,
  pub base: i32,
  pub cmbn: i32,
  pub emit: c_float,
  pub vertex_indices: [i32; 4],
  pub name: [c_char; 16],
//   char name[NAMELEN];
}

#[derive(Debug, Clone, Copy)]
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
    pub fn vf(&self, a: usize, b: usize) -> Option<f64> {
        let index = (a-1)*(self.n_surfs as usize)+(b-1);
        if index < self.values.len() {
            Some(self.values[index])
        } else {
            None
        }
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

/// Print the view factor results to Writable handle, e.g. a file open for
/// writing or stdout etc.
pub fn print_vf_results<T: Write>(handle: &mut T,  results: &VFResults) -> std::io::Result<()> {
    write!(handle, "encl: {}\n", results.encl)?;
    // Print column numbering
    write!(handle, "      ")?;
    for i in 1..=(results.n_surfs as usize) {
        write!(handle, "{:^8} ", i)?;
    }
    write!(handle, "\n")?;
    // Print areas
    write!(handle, "area: ")?;
    for area in results.areas.iter() {
        write!(handle, "{:.*} ", 6, area)?;
    }
    write!(handle, "\n")?;
    // Print emissivities
    write!(handle, "emit: ")?;
    for emit in results.emit.iter() {
        write!(handle, "{:.*} ", 6, emit)?;
    }
    write!(handle, "\n")?;
    // Print separator
    write!(handle, "      ")?;
    for _ in 1..=(results.n_surfs as usize) {
        write!(handle, "---------")?;
    }
    write!(handle, "\n")?;
    // Print view factors
    for (i, value) in results.values.iter().enumerate() {
        if i % (results.n_surfs as usize) == 0 {
            write!(handle, "{:4}: ", (i /(results.n_surfs as usize)) + 1)?;
        }
        write!(handle, "{:.*}", 6, value)?;
        if (i + 1) % (results.n_surfs as usize) == 0 {
            write!(handle, "\n")?;
        } else {
            write!(handle, " ")?;
        }
    }
    Ok(())
}

pub fn analytic_1(width: f64, height: f64) -> f64 {
    if width <= 0_f64 || height <= 0_f64 {
        panic!("Cannot use analytic_1 with non-positive values");
    }
    let w = width/height;
    let x = (1_f64+w.powi(2)).sqrt();
    let y = (w/x).atan()*x-(w).atan();
    let f12 = (1_f64/(f64::consts::PI*w.powi(2)))*((x.powi(4)/(1_f64+2_f64*w.powi(2))).ln()+4_f64*w*y);
    f12
}

/// Assert that two numerical values are within epsilon of each other
#[macro_export]
macro_rules! assert_eq_err {
    ($left:expr, $right:expr, $epsilon:expr) => ({
        match (&$left, &$right, &$epsilon) {
            (left_val, right_val, epsilon_val) => {
                if !((*left_val - *right_val).abs() < *epsilon_val) {
                    panic!(r#"assertion failed: `(left - right).abs() < epsilon`
  left: `{:?}`,
 right: `{:?}`,
  diff: `{:?}`,
   eps: `{:?}`"#, left_val, right_val, (*left_val - *right_val).abs(), epsilon_val)
                }
            }
        }
    });
    ($left:expr, $right:expr, $epsilon:expr,) => ({
        assert_eq_err!($left, $right, $epsilon)
    });
    ($left:expr, $right:expr, $($arg:tt)+) => ({
        match (&($left), &($right)) {
            (left_val, right_val) => {
                if !(*left_val == *right_val) {
                    panic!(r#"assertion failed: `(left == right)`
  left: `{:?}`,
 right: `{:?}`,
  diff: `{:?}`,
   eps: `{:?}`: {}"#, left_val, right_val, (*left_val - *right_val).abs(), epsilon_val,
                           format_args!($($arg)+))
                }
            }
        }
    });
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn matches_analytic_1a() {
        let analytic_result = analytic_1(1_f64,1_f64);
        let vf_results = process_path("examples\\ParallelPlanes.vs3".to_string());
        let numerical_result = vf_results.vf(1,2).unwrap();
        assert_eq_err!(analytic_result, numerical_result, 0.0000001);
    }

    #[test]
    fn matches_analytic_1b() {
        let analytic_result = analytic_1(1_f64,2_f64);
        let vf_results = process_path("examples\\ParallelPlanes2.vs3".to_string());
        let numerical_result = vf_results.vf(1,2).unwrap();
        assert_eq_err!(analytic_result, numerical_result, 0.0000001);
    }

    fn create_parallel_planes_example(width: f64, height: f64) -> InData {
        let mut name1 = String::new();
        name1.push_str("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
        name1.truncate(24);
        let mut name1_c : [i8; 16] = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
        let byte_name: Vec<i8> = name1.into_bytes().into_iter().map(|x| x as i8).collect();
        name1_c.clone_from_slice(&byte_name[..16]);
        let name2_c = name1_c.clone();
        InData {
            opts: InOptions {
                title: String::from("test"),
                eps_adap: 1.0e-4_f32,
                max_recurs_ali: 12,
                min_recursion: 0,
                max_recursion: 8,
                enclosure: false,
                emittances: false,
                row: 0,
                col: 0,
                prj_reverse: false,
            },
            n_all_srf: 2,
            n_rad_srf: 2,
            n_obstr_srf: 0,
            n_vertices: 8,
            vertices: vec![
                Vec3 {x: 0_f64, y: 0_f64, z: 0_f64},
                Vec3 {x: width, y: 0_f64, z: 0_f64},
                Vec3 {x: width, y: width, z: 0_f64},
                Vec3 {x: 0_f64, y: width, z: 0_f64},
                Vec3 {x: 0_f64, y: 0_f64, z: height},
                Vec3 {x: width, y: 0_f64, z: height},
                Vec3 {x: width, y: width, z: height},
                Vec3 {x: 0_f64, y: width, z: height},
                ],
            surfaces: vec![
                RawSurf { nr: 1, nv: 4, type_: 0, base: 0, cmbn: 0, emit: 0.9, vertex_indices: [1,2,3,4], name: name1_c},
                RawSurf { nr: 2, nv: 4, type_: 0, base: 0, cmbn: 0, emit: 0.9, vertex_indices: [8,7,6,5], name: name2_c},
            ],
        }
    }
    
    #[test]
    fn from_file_matches_from_code() {
        let vf_results_file = process_path("examples\\ParallelPlanes.vs3".to_string());
        let indata_code = create_parallel_planes_example(1_f64, 1_f64);
        let vf_results_code = process_v3d(indata_code);
        let vf_results_file_12 = vf_results_file.vf(1,2).unwrap();
        let vf_results_code_12 = vf_results_code.vf(1,2).unwrap();
        assert_eq!(vf_results_file_12, vf_results_code_12);
    }

    quickcheck! {
        fn matches_analytic_1_prop(width: f64, height: f64) -> TestResult {
            if width <= 0_f64 || height <= 0_f64 {
                return TestResult::discard()
            }
            print!("width: {} m, height: {} m > ", width, height);
            let analytic_result = analytic_1(width, height);
            let vf_results = process_v3d(create_parallel_planes_example(width, height));
            let numerical_result = vf_results.vf(1,2).unwrap();
            let epsilon = 0.0001_f64;
            println!("analytic: {}, numerical: {}, {:?}", analytic_result, numerical_result,((analytic_result - numerical_result).abs() < epsilon));
            TestResult::from_bool((analytic_result - numerical_result).abs() < epsilon)
        }
    }
}
