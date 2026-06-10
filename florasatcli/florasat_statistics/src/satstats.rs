use csv::Error;
use itertools::Itertools;
use rmp_serde::Serializer;
use std::{
    collections::HashMap,
    fs::{self, File},
    io::BufReader,
    path::Path,
};

use pyo3::{pyclass, pyfunction, PyResult};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug)]
struct Record {
    sat_id: u32,
    timestamp: f32,
    queue_size: u32,
}

#[pyclass(module = "Satellite", get_all)]
#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct Satellite {
    sat_id: u32,
    entries: Vec<State>,
}

#[pyclass(module = "state", get_all)]
#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct State {
    start: f32,
    qs: u32,
}

#[pyfunction]
pub fn load_sat_stats(read_path: String) -> PyResult<Vec<Satellite>> {
    let data = fs::read(read_path)?;
    let res: Vec<Satellite> = rmp_serde::from_slice(&data).expect("Unable to parse");
    Ok(res)
}

#[pyfunction]
pub fn process_sat_stats(read_path: String, file_path: String) -> PyResult<()> {
    let path_buf = Path::new(&file_path);
    let folder = path_buf.parent().unwrap();

    // read from file
    let file = File::open(read_path)?;
    let reader = BufReader::new(file);
    let mut rdr = csv::Reader::from_reader(reader);
    let iter = rdr.deserialize();

    // transform
    let satellites = transform_satstats(iter);

    // write
    let mut buf = Vec::new();
    satellites
        .serialize(&mut Serializer::new(&mut buf))
        .unwrap();
    fs::create_dir_all(folder)?;
    fs::write(path_buf, buf)?;

    Ok(())
}

fn transform_satstats(iter: impl Iterator<Item = Result<Record, Error>>) -> Vec<Satellite> {
    let mut open_statistics = HashMap::<u32, (f32, u32)>::new();
    let mut satellite_states = HashMap::<u32, Satellite>::new();
    for record in iter {
        let record: Record = record.unwrap();
        let id = record.sat_id;
        let timestamp = record.timestamp;
        let queue_size = record.queue_size;
        let (old_timestamp, old_queue_size) = open_statistics.entry(id).or_default();
        // assert!(timestamp >= *old_timestamp);
        // assert_ne!(queue_size, *old_queue_size);
        // assert!(queue_size == *old_queue_size + 1 || queue_size == *old_queue_size - 1);

        let sat = satellite_states.entry(id).or_insert(Satellite {
            sat_id: id,
            entries: vec![State { start: 0.0, qs: 0 }],
        });

        let mut i = (*old_timestamp).round() + 5.0;
        while i < timestamp {
            let state = State {
                start: i,
                qs: *old_queue_size,
            };
            sat.entries.push(state);
            i += 5.0;
        }

        let state = State {
            start: timestamp,
            qs: queue_size,
        };
        sat.entries.push(state);

        *old_timestamp = timestamp;
        *old_queue_size = queue_size;
    }
    satellite_states.values().cloned().collect_vec()
}

#[cfg(test)]
mod tests {
    use stringreader::StringReader;

    use crate::satstats::transform_satstats;

    #[test]
    fn test_transform_satstats() {
        let streader = StringReader::new(
            "sat_id,timestamp,queue_size\n\
        0,1.005243,1\n\
        1,1.005246,1\n\
        0,1.005443,2\n\
        0,1.005463,1\n\
        1,1.005563,2\n\
        0,1.005663,0\n\
        1,1.005763,3\n\
        1,1.005765,4\n\
        1,1.005773,5\n\
        0,1.005863,1\n\
        0,1.005864,2\n\
        ",
        );
        let mut rdr = csv::Reader::from_reader(streader);
        let iter = rdr.deserialize();

        // transform
        let routes = transform_satstats(iter);

        println!("{:?}", routes)
    }
}
