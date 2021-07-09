#!/bin/bash

docker run \
        	--name mqtt-wamp-bridge \
        	--restart always \
        	-v /data/mqtt-wamp-bridge/docker-run/config.ini:/data/mqtt-wamp-bridge/config.ini \
			-v /etc/localtime:/etc/localtime:ro \
			-v /etc/timezone:/etc/timezone:ro \
			-d mqtt-wamp-bridge:1.0
