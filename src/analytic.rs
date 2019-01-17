use std::f64;
use std::f64::consts::PI;

/// Calculate the view factor between two squares of side length _width_ and
/// distance apart _height_.
pub fn analytic_1(width: f64, height: f64) -> f64 {
    if width <= 0_f64 || height <= 0_f64 {
        panic!("Cannot use analytic_1 with non-positive values");
    }
    let w = width/height;
    let x = (1_f64+w.powi(2)).sqrt();
    let y = (w/x).atan()*x-(w).atan();
    let f12 = (1_f64/(PI*w.powi(2)))*((x.powi(4)
            /(1_f64+2_f64*w.powi(2))).ln()+4_f64*w*y);
    f12
}

/// Calculate the view factor between two identical rectangles with length _a_
/// and width _b_, separated by distance _c_.
pub fn analytic_c11(a: f64, b: f64, c: f64) -> f64 {
    if a <= 0_f64 || b <= 0_f64 || c <= 0_f64 {
        panic!("Cannot use analytic_c11 with non-positive values");
    }
    #[allow(non_snake_case)]
    let X = a/c;
    #[allow(non_snake_case)]
    let Y = b/c;
    let t1 = (((1_f64+X.powi(2))*(1_f64+Y.powi(2)))/(1_f64+X.powi(2)+Y.powi(2))).powf(0.5_f64);
    let t2 = X*(1_f64+Y.powi(2)).sqrt()*(X/(1_f64+Y.powi(2)).sqrt()).atan();
    let t3 = Y*(1_f64+X.powi(2)).sqrt()*(Y/(1_f64+X.powi(2)).sqrt()).atan();
    let t4 = X*X.atan();
    let t5 = Y*Y.atan();
    let f12 = 2_f64/(PI*X*Y)*(t1.ln()+t2+t3-t4-t5);
    f12
}

#[cfg(test)]
mod tests {
    use super::*;
    use super::super::*;
    use quickcheck::*;

    #[test]
    fn matches_1_c11() {
        let analytic_result_1 = analytic_1(1_f64, 1_f64);
        let analytic_result_c11 = analytic_c11(1_f64, 1_f64, 1_f64);
        assert_eq_err!(analytic_result_1, analytic_result_c11, 0.0000001);
    }

    #[test]
    #[should_panic]
    fn use_zero_values_analytic_1() {
        analytic_1(0_f64, 1_f64);
    }

    #[test]
    #[should_panic]
    fn use_negative_values_analytic_1() {
        analytic_1(-1_f64, 1_f64);
    }

    /// Test that the analytic results of 1 matches the results of c11 when
    /// they use the same geometry.
    quickcheck! {
        fn matches_1_c11_prop(width: f64, height: f64) -> TestResult {
            if width <= 0_f64 || height <= 0_f64 {
                return TestResult::discard()
            }
            print!("width: {} m, height: {} m > ", width, height);
            let analytic_result_1 = analytic_1(width, height);
            let analytic_result_c11 = analytic_c11(width, width, height);
            let epsilon = 0.0001_f64;
            println!("analytic_1: {}, analytic_c11: {}, {:?}", analytic_result_1, analytic_result_c11,((analytic_result_1 - analytic_result_c11).abs() < epsilon));
            TestResult::from_bool((analytic_result_1 - analytic_result_c11).abs() < epsilon)
        }
    }
}
