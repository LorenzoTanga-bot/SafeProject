fn processor(client_subscribe : mqtt::Client, client_publisher : mqtt::Client, rx : mpsc::Receiver<Option<Message>>){
    println!("Processing requests...");
    for msg in rx.iter() {
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
            let tok = client_publisher.publish(new_msg);

            if let Err(e) = tok {
                println!("Error sending message: {:?}", e);
                break;
            }
        } else if !client_subscribe.is_connected() {
            if try_reconnect(&client_subscribe) {
                println!("Resubscribe topics...");
                subscribe_topics(&client_subscribe);
            } else {
                break;
            }
        }
    }

    // If still connected, then disconnect now.
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