use std::f64;
use std::f64::consts::PI;

/// Calculate the view factor between two squares of side length _width_ and
/// distance apart _height_.
pub fn analytic_1(width: f64, height: f64) -> f64 {
    if width <= 0_f64 || height <= 0_f64 {
        panic!("Cannot use analytic_1 with non-positive values");
    }
    let w = width / height;
    let x = (1_f64 + w.powi(2)).sqrt();
    let y = (w / x).atan() * x - (w).atan();
    let f12 = (1_f64 / (PI * w.powi(2)))
        * ((x.powi(4) / (1_f64 + 2_f64 * w.powi(2))).ln() + 4_f64 * w * y);
    f12
}

/// Calculate the view factor between two identical rectangles with length _a_
/// and width _b_, separated by distance _c_.
pub fn analytic_c11(a: f64, b: f64, c: f64) -> f64 {
    if a <= 0_f64 || b <= 0_f64 || c <= 0_f64 {
        panic!("Cannot use analytic_c11 with non-positive values");
    }
    #[allow(non_snake_case)]
    let X = a / c;
    #[allow(non_snake_case)]
    let Y = b / c;
    let t1 = (((1_f64 + X.powi(2)) * (1_f64 + Y.powi(2))) / (1_f64 + X.powi(2) + Y.powi(2)))
        .powf(0.5_f64);
    let t2 = X * (1_f64 + Y.powi(2)).sqrt() * (X / (1_f64 + Y.powi(2)).sqrt()).atan();
    let t3 = Y * (1_f64 + X.powi(2)).sqrt() * (Y / (1_f64 + X.powi(2)).sqrt()).atan();
    let t4 = X * X.atan();
    let t5 = Y * Y.atan();
    let f12 = 2_f64 / (PI * X * Y) * (t1.ln() + t2 + t3 - t4 - t5);
    f12
}

/// From http://www.thermalradiation.net.
/// C-15: Rectangle to rectangle in a perpendicular plane; all boundaries are
/// parallel or perpendicular to x and ξ boundaries.(Form revised from Ehlert
/// and Smith) (Note that the expression fails if the rectangles share a common
/// edge)
/// \[
/// F_{12}=\frac{1}{\left(x_{2}-x_{1}\right)\left(y_{2}-y_{1}\right)}
/// \sum_{l=1}^{2}\sum_{k=1}^{2}\sum_{k=1}^{2}\sum_{j=1}^{2}\left[
/// \left(-1\right)^{\left(i+j+k+l\right)}G\left(x_{i},y_{j},\eta_{k},
/// \xi_{l}\right)\right]
/// \]
pub fn analytic_c15(
    x1: f64,
    x2: f64,
    y1: f64,
    y2: f64,
    eta1: f64,
    eta2: f64,
    xi1: f64,
    xi2: f64,
) -> f64 {
    // Check for validity of values
    if x1 < 0_f64
        || x2 < 0_f64
        || y1 < 0_f64
        || y2 < 0_f64
        || eta1 < 0_f64
        || eta2 < 0_f64
        || xi1 < 0_f64
        || xi2 < 0_f64
    {
        panic!("Cannot use analytic_c15 with non-positive values");
    }
    if x2 <= x1 {
        panic!("x2 must be greater than x1");
    }
    if y2 <= y1 {
        panic!("y2 must be greater than y1");
    }
    if eta2 <= eta1 {
        panic!("eta2 must be greater than eta1");
    }
    if xi2 <= xi1 {
        panic!("xi2 must be greater than xi1");
    }
    // We want to sum over every combination of 1/2 in x, y, eta, and xi
    let indices: Vec<[i32; 4]> = vec![
        [1, 1, 1, 1],
        [1, 1, 1, 2],
        [1, 1, 2, 1],
        [1, 1, 2, 2],
        [1, 2, 1, 1],
        [1, 2, 1, 2],
        [1, 2, 2, 1],
        [1, 2, 2, 2],
        [2, 1, 1, 1],
        [2, 1, 1, 2],
        [2, 1, 2, 1],
        [2, 1, 2, 2],
        [2, 2, 1, 1],
        [2, 2, 1, 2],
        [2, 2, 2, 1],
        [2, 2, 2, 2],
    ];
    let tht: f64 = indices
        .into_iter()
        .map(|is| analytic_c15_fd([x1, x2], [y1, y2], [eta1, eta2], [xi1, xi2], is))
        .sum();
    (1_f64 / ((x2 - x1) * (y2 - y1))) * tht
}

fn analytic_c15_fd(
    xs: [f64; 2],
    ys: [f64; 2],
    etas: [f64; 2],
    xis: [f64; 2],
    indices: [i32; 4],
) -> f64 {
    let [i, j, k, l] = indices;
    (-1_f64).powi(i + j + k + l)
        * (analytic_c15_g(
            xs[(i - 1) as usize],
            ys[(j - 1) as usize],
            etas[(k - 1) as usize],
            xis[(l - 1) as usize],
        ))
}

fn analytic_c15_g(x: f64, y: f64, eta: f64, xi: f64) -> f64 {
    let k = (y - eta) / ((x.powi(2) + xi.powi(2)).powf(0.5_f64));
    (1_f64 / (2_f64 * std::f64::consts::PI))
        * ((y - eta) * ((x.powi(2) + xi.powi(2)).powf(0.5)) * k.atan()
            - (1_f64 / 4_f64)
                * ((x.powi(2) + xi.powi(2)) * ((1_f64 + k.powi(2)).ln())
                    - (y - eta).powi(2) * ((1_f64 + (1_f64 / k.powi(2))).ln())))
}

pub fn analytic_coaxial_squares(width1: f64, width2: f64, distance: f64) -> f64 {
    let w1 = width1/distance;
    let w2 = width2/distance;
    let p = (w1.powi(2)+w2.powi(2)+2_f64).powi(2);
    let x = w2-w1;
    let y = w2+w1;
    let q = (x.powi(2)+2_f64)*(y.powi(2)+2_f64);
    let u = (x.powi(2)+4_f64).sqrt();
    let v = (y.powi(2)+4_f64).sqrt();
    let s = u*(x*(x/u).atan()-y*(y/u).atan());
    let t = v*(x*(x/v).atan()-y*(y/v).atan());
    (1_f64/(std::f64::consts::PI*w1.powi(2)))*((p/q).ln()+s-t)
}

#[cfg(test)]
mod tests {
    use super::super::*;
    use super::*;
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

    // Test that the analytic results of 1 matches the results of c11 when
    // they use the same geometry.
    quickcheck! {
        fn matches_1_c11_prop(width: f64, height: f64) -> TestResult {
            if width <= 0_f64 || height <= 0_f64 {
                return TestResult::discard()
            }
            print!("width: {} m, height: {} m > ", width, height);
            let analytic_result_1 = analytic_1(width, height);
            let analytic_result_c11 = analytic_c11(width, width, height);
            let epsilon = 0.0001_f64;
            println!("analytic_1: {}, analytic_c11: {}, {:?}",
                analytic_result_1,
                analytic_result_c11,
                ((analytic_result_1 - analytic_result_c11).abs() < epsilon));
            TestResult::from_bool(
                (analytic_result_1 - analytic_result_c11).abs() < epsilon)
        }
    }

    #[test]
    fn use_analytic_c15() {
        let res = analytic_c15(3_f64, 4_f64, 3_f64, 4_f64, 1_f64, 2_f64, 1_f64, 2_f64);
        println!("analytic_c15: {}", res);
        let expected = 4.8504066904213605e-3_f64;
        assert_eq_err!(res, expected, 0.0000001);
    }

    #[test]
    fn use_analytic_c15_symmetric_a() {
        let res = analytic_c15(3_f64, 4_f64, 5_f64, 6_f64, 3_f64, 4_f64, 3_f64, 4_f64);
        println!("analytic_c15_a: {}", res);
        let expected = 0.004768347753645874_f64;
        assert_eq_err!(res, expected, 0.0000001);
    }

    /// Same as use_analytic_c15_symmetric_a except one face is on the other
    /// side. The view factor value should be the same
    #[test]
    fn use_analytic_c15_symmetric_b() {
        let res = analytic_c15(3_f64, 4_f64, 1_f64, 2_f64, 3_f64, 4_f64, 3_f64, 4_f64);
        println!("analytic_c15_b: {}", res);
        let expected = 0.004768347753645874_f64;
        assert_eq_err!(res, expected, 0.0000001);
    }

     #[test]
    fn use_analytic_coaxial_squares() {
        let res = analytic_coaxial_squares(1_f64,1_f64,1_f64);
        println!("analytic_coaxial_squares: {}", res);
        let expected = 0.19982489569838732_f64;
        assert_eq_err!(res, expected, 0.0000001);
    }

    #[test]
    fn use_analytic_coaxial_squares2() {
        let res = analytic_coaxial_squares(1_f64,1_f64,1_f64);
        println!("analytic_coaxial_squares: {}", res);
        let expected = 0.19982489569838732_f64;
        assert_eq_err!(res, expected, 0.0000001);
    }
}
