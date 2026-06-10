use pyo3::{pymodule, types::PyModule, wrap_pyfunction, PyResult, Python};
use routes::{load_routes, process_routes, Hop, Route};
use satstats::{load_sat_stats, process_sat_stats, Satellite, State};

pub mod routes;
pub mod satstats;

/// A Python module implemented in Rust.
#[pymodule]
fn florasat_statistics(_py: Python, m: &PyModule) -> PyResult<()> {
    m.add_class::<Hop>()?;
    m.add_class::<Route>()?;
    m.add_function(wrap_pyfunction!(load_routes, m)?)?;
    m.add_function(wrap_pyfunction!(process_routes, m)?)?;

    m.add_class::<Satellite>()?;
    m.add_class::<State>()?;
    m.add_function(wrap_pyfunction!(load_sat_stats, m)?)?;
    m.add_function(wrap_pyfunction!(process_sat_stats, m)?)?;

    Ok(())
}
