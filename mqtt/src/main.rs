use chrono::prelude::*;
use std::{process, thread, time::Duration};
use uuid::Uuid;

extern crate paho_mqtt as mqtt;
extern crate serde_json;
use serde_json::{json, Value as JsonValue};

const DFLT_BROKER: &str = "tcp://localhost:1883";
const DFLT_CLIENT: &[&str] = &["rust_subscribe", "rust_publish"];
const DFLT_TOPICS: &[&str] = &[
    "application/1/device/+/event/up",
    "safe.it/jz/device/snapshot/0000/edv/",
];
// Define the qos.
const QOS: i32 = 1;
// Reconnect to the broker when connection is lost.
fn try_reconnect(cli: &mqtt::Client) -> bool {
    println!("Connection lost. Waiting to retry connection");
    for _ in 0..12 {
        thread::sleep(Duration::from_millis(5000));
        if cli.reconnect().is_ok() {
            println!("Successfully reconnected");
            return true;
        }
    }
    println!("Unable to reconnect after several attempts.");
    false
}

fn subscribe_topics(cli: &mqtt::Client) {
    if let Err(e) = cli.subscribe(DFLT_TOPICS[0], QOS) {
        println!("Error subscribes topics: {:?}", e);
        process::exit(1);
    }
}

fn main() {
    // Define the set of options for the create.
    // Use an ID for a persistent session.
    let create_opts = mqtt::CreateOptionsBuilder::new()
        .server_uri(DFLT_BROKER.to_string())
        .client_id(DFLT_CLIENT[0].to_string())
        .finalize();

    // Create a client.
    let mut cli = mqtt::Client::new(create_opts).unwrap_or_else(|err| {
        println!("Error creating the client: {:?}", err);
        process::exit(1);
    });

    // Initialize the consumer before connecting.
    let rx_sub = cli.start_consuming();

    // Define the set of options for the connection.
    let conn_opts = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(20))
        .clean_session(false)
        .finalize();

    // Connect and wait for it to complete or fail.
    if let Err(e) = cli.connect(conn_opts) {
        println!("Unable to connect:\n\t{:?}", e);
        process::exit(1);
    }

    // Subscribe topics.
    subscribe_topics(&cli);

    println!("Processing requests...");
    for msg in rx_sub.iter() {
        if let Some(msg) = msg {
            let content = msg
                .to_string()
                .replace("application/1/device/3036363283397307/event/up: ", "");
            let json_content: JsonValue = serde_json::from_str(&content).unwrap();

            let profile: i32 = json_content["object"]["sensor"]["Profile"]
                .to_string()
                .parse::<i32>()
                .unwrap();
            let state_code: i32 = json_content["object"]["sensor"]["StateCode"]
                .to_string()
                .parse::<i32>()
                .unwrap();
            let time: DateTime<Local> = Local::now();
            let uuid = Uuid::new_v4();
            let reff: &str = &format!(
                "jzp/{}.0000",
                json_content["devEUI"].to_string().replace("\"", "")
            );
            println!(
                "{} / {} / {} / {} / {}",
                profile, state_code, time, uuid, reff
            );

            let new_content: JsonValue;
            if (profile == 0 && state_code == 0) || (profile == 1 && state_code < 3) {
                new_content = json!({
                "tx": time.to_string(),
                "uuid": uuid.to_string(),
                "ref": reff.to_string(),
                "type": "presence_uwb",
                "m": [
                    {
                        "tx": time.to_string(),
                        "k": "safe_status",
                        "v": json_content["object"]["sensor"]["Profile"].to_string(),
                    },
                    {
                        "tx": time.to_string(),
                        "k": "presence",
                        "v": json_content["object"]["sensor"]["StateCode"],
                    },
                    {
                        "tx": time.to_string(),
                        "k": "distance",
                        "v": json_content["object"]["sensor"]["Distance"],
                        "u": "m",
                    },
                    {
                        "tx": time.to_string(),
                        "k": "rpm",
                        "v": json_content["object"]["sensor"]["Rpm"],
                    },
                    {
                        "k": "movement",
                        "v": json_content["object"]["sensor"]["Movement"],
                        "u": "mm",
                    },
                    {
                        "tx": time.to_string(),
                        "k": "signalquality",
                        "v": json_content["object"]["sensor"]["SignalQuality"],
                    }
                ]});
            } else {
                new_content = json!({
                "tx": time.to_string(),
                "uuid": uuid.to_string(),
                "ref": reff.to_string(),
                "type": "presence_uwb",
                "m": [
                    {
                        "tx": time.to_string(),
                        "k": "safe_status",
                        "v": json_content["object"]["sensor"]["Profile"].to_string(),
                    },
                    {
                        "tx": time.to_string(),
                        "k": "presence",
                        "v": json_content["object"]["sensor"]["StateCode"],
                    }
                ]});
            }

            let new_msg = mqtt::Message::new(DFLT_TOPICS[1], new_content.to_string().clone(), QOS);
            let tok = cli.publish(new_msg);

            if let Err(e) = tok {
                println!("Error sending message: {:?}", e);
                break;
            }
        } else if !cli.is_connected() {
            if try_reconnect(&cli) {
                println!("Resubscribe topics...");
                subscribe_topics(&cli);
            } else {
                break;
            }
        }
    }

    // If still connected, then disconnect now.
    if cli.is_connected() {
        println!("Disconnecting");
        cli.unsubscribe_many(DFLT_TOPICS).unwrap();
        cli.disconnect(None).unwrap();
    }
    println!("Exiting");
}
