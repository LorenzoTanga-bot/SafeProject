use std::{env, process, thread, time::Duration};

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

fn main() {
    // Define the set of options for the create.
    // Use an ID for a persistent session.
    let create_opts_sub = mqtt::CreateOptionsBuilder::new()
        .server_uri(DFLT_BROKER.to_string())
        .client_id(DFLT_CLIENT[0].to_string())
        .finalize();

    let create_opts_pub = mqtt::CreateOptionsBuilder::new()
        .server_uri(DFLT_BROKER.to_string())
        .client_id(DFLT_CLIENT[1].to_string())
        .finalize();

    // Create a client.
    let mut cli_sub = mqtt::Client::new(create_opts_sub).unwrap_or_else(|err| {
        println!("Error creating the client: {:?}", err);
        process::exit(1);
    });

    let mut cli_pub = mqtt::Client::new(create_opts_pub).unwrap_or_else(|err| {
        println!("Error creating the client: {:?}", err);
        process::exit(1);
    });

    // Initialize the consumer before connecting.
    let rx_sub = cli_sub.start_consuming();

    // Define the set of options for the connection.
    let lwt = mqtt::MessageBuilder::new()
        .topic("test")
        .payload("Consumer lost connection")
        .finalize();
    let conn_opts_sub = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(20))
        .clean_session(false)
        .will_message(lwt)
        .finalize();

    let conn_opts_pub = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(20))
        .clean_session(true)
        .finalize();

    // Connect and wait for it to complete or fail.
    if let Err(e) = cli_sub.connect(conn_opts_sub) {
        println!("Unable to connect:\n\t{:?}", e);
        process::exit(1);
    }

    if let Err(e) = cli_pub.connect(conn_opts_pub) {
        println!("Unable to connect:\n\t{:?}", e);
        process::exit(1);
    }

    // Subscribe topics.
    subscribe_topics(&cli_sub);

    println!("Processing requests...");
    for msg in rx_sub.iter() {
        if let Some(msg) = msg {
            let content = msg
                .to_string()
                .replace("application/1/device/3036363283397307/event/up: ", "");
            let json_content: JsonValue = serde_json::from_str(&content).unwrap();
            let new_content: JsonValue = json!({
            "type": "presence_uwb",
            "m": [
                {
                    "k": "safe_status",
                    "v": json_content["object"]["sensor"]["Profile"].to_string(),
                },
                {
                    "k": "presence",
                    "v": json_content["object"]["sensor"]["StateCode"],
                }
            ]});
            let new_msg = mqtt::Message::new(DFLT_TOPICS[1], new_content.to_string().clone(), QOS);
            let tok = cli_pub.publish(new_msg);

            if let Err(e) = tok {
                println!("Error sending message: {:?}", e);
                break;
            }
        } else if !cli_sub.is_connected() {
            if try_reconnect(&cli_sub) {
                println!("Resubscribe topics...");
                subscribe_topics(&cli_sub);
            } else {
                break;
            }
        }
    }

    // If still connected, then disconnect now.
    if cli_sub.is_connected() {
        println!("Disconnecting");
        cli_sub.unsubscribe_many(DFLT_TOPICS).unwrap();
        cli_sub.disconnect(None).unwrap();
    }
    println!("Exiting");
}
