use std::{process, thread, time::Duration};

extern crate paho_mqtt as mqtt;
extern crate serde_json;
use serde_json::{json, Value as JsonValue};

const DFLT_BROKER: &str = "tcp://localhost:1883";
const DFLT_CLIENT: &[&str] = &["rust_subscribe", "rust_publish"];
const DFLT_TOPICS: &[&str] = &[
    "application/1/device/+/event/up",
    "safe.it/jz/device/snapshot/0000/edv/",
];

const QOS: i32 = 1;

fn create_client_subscribe(broker: &str, client: &str) -> mqtt::Client{
    let create_opts = mqtt::CreateOptionsBuilder::new()
        .server_uri(broker.to_string())
        .client_id(client.to_string())
        .finalize();

    let client = mqtt::Client::new(create_opts).unwrap_or_else(|err| {
        println!("Error creating the client: {:?}", err);
        process::exit(1);
    });

    return client;
}

fn create_connection_subscribe(client: mqtt::Client, topik: &str){
    let lwt = mqtt::MessageBuilder::new()
        .topic("test")
        .payload("Consumer lost connection")
        .finalize();
    let conn_opts = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(20))
        .clean_session(false)
        .will_message(lwt)
        .finalize();
    
    if let Err(e) = client.connect(conn_opts) {
        println!("Unable to connect:\n\t{:?}", e);
        process::exit(1);
    }

    if let Err(e) = client.subscribe(topik, QOS) {
        println!("Error subscribes topics: {:?}", e);
        process::exit(1);
    }
}

fn create_client_publisher(broker: &str, client: &str) -> mqtt::Client{
    let create_opts = mqtt::CreateOptionsBuilder::new()
        .server_uri(broker.to_string())
        .client_id(client.to_string())
        .finalize();

    let client = mqtt::Client::new(create_opts).unwrap_or_else(|err| {
        println!("Error creating the client: {:?}", err);
        process::exit(1);
    });

    return client;
}

fn create_connection_publisher(client: mqtt::Client){
    let conn_opts = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(20))
        .clean_session(true)
        .finalize();
    
    
    if let Err(e) = client.connect(conn_opts) {
        println!("Unable to connect:\n\t{:?}", e);
        process::exit(1);
    }
}

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

fn analys(content: &str) -> JsonValue{
    let json_content: JsonValue = serde_json::from_str(&content).unwrap();
    return json!({
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
}

fn disconnect(client_subscribe : mqtt::Client, client_publisher : mqtt::Client){
    if client_subscribe.is_connected() {
        println!("Disconnecting");
        client_subscribe.unsubscribe(DFLT_TOPICS[0]).unwrap();
        client_subscribe.disconnect(None).unwrap();
    }

    if client_publisher.is_connected() {
        println!("Disconnecting");
        client_publisher.disconnect(None).unwrap();
    }
}

fn main(){
    let client_subscribe : mqtt::Client = create_client_subscribe(DFLT_BROKER, DFLT_CLIENT[0]);
    let client_publisher : mqtt::Client = create_client_publisher(DFLT_BROKER, DFLT_CLIENT[1]);
    let rx = client_subscribe.start_consuming();
    create_connection_subscribe(client_subscribe, DFLT_TOPICS[0]);
    create_connection_publisher(client_publisher);

    println!("Processing requests...");
    for msg in rx.iter() {
        if let Some(msg) = msg {
            let content = msg
                .to_string()
                .replace("application/1/device/3036363283397307/event/up: ", "");
            
            let new_content: JsonValue = analys(&content);
            let new_msg = mqtt::Message::new(DFLT_TOPICS[1], new_content.to_string().clone(), QOS);
            let tok = client_publisher.publish(new_msg);

            if let Err(e) = tok {
                println!("Error sending message: {:?}", e);
                break;
            }
        } else if !client_subscribe.is_connected() {
            if try_reconnect(&client_subscribe) {
                println!("Resubscribe topics...");
                if let Err(e) = client_subscribe.subscribe(DFLT_TOPICS[0], QOS) {
                    println!("Error subscribes topics: {:?}", e);
                    process::exit(1);
                }
            } else {
                break;
            }
        }
    }

    
    println!("Exiting");

}

