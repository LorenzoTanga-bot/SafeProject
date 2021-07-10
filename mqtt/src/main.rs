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


fn main(){
    let client_subscribe : mqtt::Client = create_client_subscribe(DFLT_BROKER, DFLT_CLIENT[0]);
    let client_publisher : mqtt::Client = create_client_publisher(DFLT_BROKER, DFLT_CLIENT[1]);
    let rx : mpsc::Receiver<Option<Message>> = client_subscribe.start_consuming();
    create_connection_subscribe(client_subscribe, DFLT_TOPICS[0]);
    create_connection_publisher(client_publisher, DFLT_TOPICS[1]);

    processor(client_subscribe, client_publisher, rx);

    if client_subscribe.is_connected() {
        println!("Disconnecting");
        client_subscribe.unsubscribe_many(DFLT_TOPICS).unwrap();
        client_subscribe.disconnect(None).unwrap();
    }

    if client_publisher.is_connected() {
        println!("Disconnecting");
        client_publisher.disconnect(None).unwrap();
    }
    println!("Exiting");

}

