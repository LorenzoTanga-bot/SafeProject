#!/bin/bash
echo "Deleting the 'mqtt-wamp-bridge' container."

docker stop mqtt-wamp-bridge
docker rm mqtt-wamp-bridge
