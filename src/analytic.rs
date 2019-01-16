use std::f64;

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