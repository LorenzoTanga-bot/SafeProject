#!/usr/bin/python
# -*- coding: utf-8 -*-

# Copyright (c) 2013 Roger Light <roger@atchoo.org>
#
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Distribution License v1.0
# which accompanies this distribution.
#
# The Eclipse Distribution License is available at
#   http://www.eclipse.org/org/documents/edl-v10.php.
#
# Contributors:
#    Roger Light - initial implementation

# This example shows how you can use the MQTT client in a class.

import paho.mqtt.client as mqtt
import configparser
import json
import logging
from utils.wampSp import TextWAMP
from datetime import datetime

# Logs
logging.basicConfig(level=logging.INFO)

# Read config.ini
config = configparser.ConfigParser()
config.read('./config.ini')
params = config["DEFAULT"]

# Wamp session
wampSession = None
wampConnectionEnum = {"NOT_CONNECTED" : 0, "CONNECTING" : 1, "CONNECTED" : 2}
wampConnectionStatus = wampConnectionEnum["NOT_CONNECTED"]


class MyMQTTClass(mqtt.Client) :
    def on_connect(self, mqttc, obj, flags, rc) :
        logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) +
                     " Connected to local MQTT broker : " + str(rc))
        self.subscribe(params["mqtt_topic"])

    def on_message(self, mqttc, obj, msg) :
        print('ho ricevuto in messaggio')
        # if (wampSession is not None) and (wampConnectionStatus == wampConnectionEnum["CONNECTED"]) :
        #     logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) +
        #                  " Lorawan message received on MQTT topic: "+params["mqtt_topic"])
        # Get Lorawan JSON
        lorawanJson = json.loads(msg.payload)
        # If 'object' attribute is not empty
        print(lorawanJson)
        
        obj = lorawanJson['object']
        sensorInfo = obj['sensor']
        print(sensorInfo)

        # Get source node id (initiator)
        sourceNodeId = lorawanJson['devEUI']
        
        # Get application type
        type = lorawanJson['applicationName']        
        
        mqttText = TextWAMP()
        mqttText.setM("safe_status", sensorInfo["Profile"])
        mqttText.setM("presence", sensorInfo["StateCode"])
        mqttText.setM("distance", sensorInfo["Distance"])
        mqttText.setM("movement", sensorInfo["Movement"])
        mqttText.setM("rpm", sensorInfo["Rpm"])
        mqttText.setM("signal_quality", sensorInfo["SignalQuality"])

        snapshot = json.dumps(mqttText.getSnapshot(sourceNodeId, type))
        
        print(snapshot)
        
        self.publish('/fratek', snapshot)
            
            # Iterate responders
            # for responder in lorawanJson['object']['sensors'] :
            #     destinationNodeId = responder['destinationNode']
            #     distanceFromInitiator = responder['distance']
            #     mqttText = TextWAMP()
            #     mqttText.setM("distance", distanceFromInitiator)
            #     snapshot = json.dumps(mqttText.getSnapshot(sourceNodeId, type))
            #     print(snapshot)
            #     self.publish('/fratek', snapshot)
            #     # self.publish(''+ params["device_id"]+ '.0000', snapshot)
            #     logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) + " SP message published to MQTT topic: "+params["wamp_topic"])
            #     logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) +
            #                      " Source node: "+str(sourceNodeId) +
            #                      " - Destination node: " + str(destinationNodeId) +
            #                      " - Distance: "+str(distanceFromInitiator))
                    # wampText = TextWAMP()
                    # wampText.setM("distance", distanceFromInitiator)
                    # snapshot = json.dumps(wampText.getSnapshot(params["device_id"], sourceNodeId, destinationNodeId))
                    # getWampSession().publish(params["wamp_topic"], snapshot)
                    # logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) +
                    #              " SP message published to WAMP topic: "+params["wamp_topic"])
                    # logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) +
                    #              " Source node: "+str(sourceNodeId) +
                    #              " - Destination node: " + str(destinationNodeId) +
                    #              " - Distance: "+str(distanceFromInitiator))

    def on_publish(self, mqttc, obj, mid) :
        logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) +
                     " mid: " + str(mid))

    def on_subscribe(self, mqttc, obj, mid, granted_qos) :
        logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) +
                     " Subscribed: " + str(mid) + " " + str(granted_qos))

    def on_log(self, mqttc, obj, level, string) :
        logging.info(str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) + " " + string)

    def run(self, host, port) :
        self.host = host
        self.port = port

        self.connect(self.host, self.port, 60)
        self.loop_start()

def setWampSession(newWampSession) :
    global wampSession
    wampSession = newWampSession


def getWampSession() :
    return wampSession


def setWampConnectionStatus(newWampConnectionStatus) :
    global wampConnectionStatus
    wampConnectionStatus = newWampConnectionStatus


def getWampConnectionStatus() :
    return wampConnectionStatus


def getParams() :
    return params
