extern crate csv;
extern crate env_logger;
extern crate log;
extern crate serde;
extern crate smartcore;
extern crate hyper;
extern crate tokio;

#[macro_use]
extern crate serde_derive;

use std::io::{Read};

use hyper::{Body, Request, Response, Server, Method, StatusCode};
use hyper::service::{make_service_fn, service_fn};
use smartcore::ensemble::random_forest_classifier::*;
use smartcore::linalg::naive::dense_matrix::DenseMatrix;
use smartcore::metrics::*;
use std::collections::HashSet;
use std::error::Error;
use std::process;

use std::fs::File;
use std::env;

use rand::Rng;
use std::collections::HashMap;
use serde_json::json;

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "PascalCase")]
struct Record {
    #[serde(rename = "Timestamp")]
    timestamp: Option<String>, // Handle missing or malformed data
    #[serde(rename = "IP Address")]
    ip_address: String,
    browser: String,
    #[serde(rename = "Request Count")]
    request_count: u32,
    #[serde(rename = "HTTP Method")]
    http_method: String,
    #[serde(rename = "Status Code")]
    status_code: u32,
    #[serde(rename = "Response Time (ms)")]
    response_time_ms: u32,
    #[serde(rename = "DDoS Happening")]
    ddos_happening: String, // This field is for classification
}

fn run_ml() -> Result<(), Box<dyn Error>> {
    // Initialize the logger
    env_logger::init();

    let file_path = "../../ddos_data.csv";
    let mut rdr = csv::Reader::from_path(file_path)?;
    println!("Reading data from {}", file_path);

    let mut data = Vec::new();
    let mut labels = Vec::new();

    for result in rdr.deserialize() {
        let record: Record = result?;
        println!("Processing record: {:?}", record);

        let browser = match record.browser.as_str() {
            "Safari" => 0.0,
            "Chrome" => 1.0,
            "Opera" => 2.0,
            _ => 3.0,
        };

        let http_method = match record.http_method.as_str() {
            "GET" => 0.0,
            "POST" => 1.0,
            "HEAD" => 2.0,
            "OPTIONS" => 3.0,
            _ => 4.0,
        };

        let ddos_label = match record.ddos_happening.as_str() {
            "NoDDoS" => "NoDDoS".to_string(),
            "DDoS" | "DDOS" => "DDoS".to_string(), // Handle both variations
            _ => {
                println!(
                    "Skipping record with unknown DDoS label: {}",
                    record.ddos_happening
                );
                continue;
            }
        };

        let row = vec![
            record.request_count as f64,
            record.status_code as f64,
            browser,
            http_method,
        ];
        data.push(row);
        labels.push(ddos_label);
    }

    if data.is_empty() || labels.is_empty() {
        println!("No data found in CSV or labels are empty.");
        return Ok(());
    }

    // Print unique labels to debug
    let unique_labels: HashSet<String> = labels.iter().cloned().collect();
    println!("Unique labels: {:?}", unique_labels);

    // Check if we have at least 2 classes
    if unique_labels.len() < 2 {
        println!(
            "Error: Not enough unique classes for classification. Found {} unique classes.",
            unique_labels.len()
        );
        return Ok(());
    }

    let nrows = data.len();
    let ncols = data[0].len();
    let data = data.into_iter().flatten().collect::<Vec<f64>>();
    let data = DenseMatrix::from_vec(nrows, ncols, &data);

    let y = labels
        .into_iter()
        .filter_map(|label| match label.as_str() {
            "NoDDoS" => Some(0.0),
            "DDoS" => Some(1.0),
            _ => {
                println!("Error: Unexpected label: {}", label);
                None
            }
        })
        .collect::<Vec<f64>>();

    println!("Training Random Forest Classifier...");
    let rfc = RandomForestClassifier::fit(&data, &y, Default::default())?;

    let y_hat = rfc.predict(&data)?;
    let accuracy = accuracy(&y, &y_hat);
    println!("Model accuracy: {}", accuracy);

    Ok(())
}

#[tokio::main]
async fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        println!("Please provide an argument: 'ml' or 'server'");
        process::exit(1);
    }

    let result = match args[1].as_str() {
        "ml" => run_ml(),
        "server" => run_server().await,
        _ => {
            println!("Invalid argument. Please provide 'ml' or 'server'");
            process::exit(1);
        }
    };

    if let Err(err) = result {
        println!("Application error: {}", err);
        process::exit(1);
    }
}

async fn handle_request(req: Request<Body>) -> Result<Response<Body>, hyper::Error> {
    let path = req.uri().path();

    if req.method() == Method::GET {
        match path {
            "/" => {
                let html_path = "static/index.html"; // Update with the correct path
                let mut file = File::open(html_path).expect("Unable to open file");
                let mut contents = String::new();
                file.read_to_string(&mut contents).expect("Unable to read file");

                Ok(Response::builder()
                    .header("Content-Type", "text/html")
                    .body(Body::from(contents))
                    .unwrap())
            },
            "/styles.css" => {
                let css_path = "static/styles.css"; // Update with the correct path
                let mut file = File::open(css_path).expect("Unable to open file");
                let mut contents = String::new();
                file.read_to_string(&mut contents).expect("Unable to read file");

                Ok(Response::builder()
                    .header("Content-Type", "text/css")
                    .body(Body::from(contents))
                    .unwrap())
            },
            "/script.js" => {
                let js_path = "static/script.js"; // Update with the correct path
                let mut file = File::open(js_path).expect("Unable to open file");
                let mut contents = String::new();
                file.read_to_string(&mut contents).expect("Unable to read file");

                Ok(Response::builder()
                    .header("Content-Type", "application/javascript")
                    .body(Body::from(contents))
                    .unwrap())
            },
            "/pow" => {
                // Handle proof-of-work challenge
                let question = generate_random_question();
                let response_body = json!({
                    "question": question
                }).to_string();

                Ok(Response::builder()
                    .header("Content-Type", "application/json")
                    .body(Body::from(response_body))
                    .unwrap())
            },
            "/check" => {
                // Handle proof-of-work check
                let body = hyper::body::to_bytes(req.into_body()).await?;
                let body_str = std::str::from_utf8(&body).unwrap();
                let body_json: serde_json::Value = serde_json::from_str(body_str).unwrap(); // Explicitly specify the type
                let answer = body_json["answer"].as_str().unwrap();

                let is_valid = check_proof_of_work(answer);
                let response_body = json!({
                    "valid": is_valid
                }).to_string();

                Ok(Response::builder()
                    .header("Content-Type", "application/json")
                    .body(Body::from(response_body))
                    .unwrap())
            },
            _ => {
                Ok(Response::builder()
                    .status(StatusCode::NOT_FOUND)
                    .body(Body::from("Not Found"))
                    .unwrap())
            },
        }
    } else {
        Ok(Response::builder()
            .status(StatusCode::METHOD_NOT_ALLOWED)
            .body(Body::from("Method Not Allowed"))
            .unwrap())
    }
}

fn generate_random_question() -> String {
    let mut rng = rand::thread_rng();
    let num1: i32 = rng.gen_range(1..10);
    let num2: i32 = rng.gen_range(1..10);

    format!("What is {} + {}?", num1, num2)
}

fn check_proof_of_work(answer: &str) -> bool {
    let parts: Vec<&str> = answer.split_whitespace().collect();
    if parts.len() != 3 {
        return false;
    }

    let num1: i32 = parts[0].parse().unwrap();
    let num2: i32 = parts[2].parse().unwrap();
    let result: i32 = parts[4].parse().unwrap();

    num1 + num2 == result
}

async fn run_server() -> Result<(), Box<dyn std::error::Error>> {
    let make_svc = make_service_fn(|_conn| async { Ok::<_, hyper::Error>(service_fn(handle_request)) });

    let addr = ([127, 0, 0, 1], 3000).into();
    let server = Server::bind(&addr).serve(make_svc);

    println!("Listening on http://{}", addr);

    server.await?;

    Ok(())
}
