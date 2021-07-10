const QOS: i32 = 1;

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

fn create_connection_publisher(client: mqtt::Client, topik: &str){
    let conn_opts = mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(20))
        .clean_session(true)
        .finalize();
    
    
    if let Err(e) = client.connect(conn_opts) {
        println!("Unable to connect:\n\t{:?}", e);
        process::exit(1);
    }
}