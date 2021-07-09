import txaio

txaio.use_twisted()
from twisted.internet import ssl
from autobahn.wamp.types import ComponentConfig
from autobahn.twisted.wamp import ApplicationSession, ApplicationRunner
from mqttCs import MyMQTTClass
from mqttCs import setWampConnectionStatus
from mqttCs import setWampSession
from mqttCs import wampConnectionEnum
from mqttCs import getParams
import logging

# Logs
logging.basicConfig(level=logging.INFO)

params = getParams()
print("Connecting to MQTT server {} {}".format(params["mqtt_host"], int(params["mqtt_port"])))
mqtt_client = MyMQTTClass()
mqtt_client.run(params["mqtt_host"], int(params["mqtt_port"]))


class MyAppSession(ApplicationSession) :
    def __init__(self, config) :
        ApplicationSession.__init__(self, config)

    def onConnect(self) :
        logging.info(' WAMP Transport connected')
        # lets join a realm .. normally, we would also specify
        # how we would like to authenticate here
        self.join(self.config.realm)

    def onChallenge(self, challenge) :
        logging.info(' WAMP: Authentication challenge received')

    def onJoin(self, details) :
        logging.info(' WAMP Session joined: {}'.format(details))
        setWampSession(self)
        setWampConnectionStatus(wampConnectionEnum["CONNECTED"])

    def onLeave(self, details) :
        logging.info(' WAMP Session left: {}'.format(details))
        setWampConnectionStatus(wampConnectionEnum["NOT_CONNECTED"])

    def onDisconnect(self) :
        logging.info(' WAMP Transport disconnected')


if __name__ == '__main__' :
    if params["log_level"] is not None :
        txaio.start_logging(level=params["log_level"])
    # create a WAMP session object. this is reused across multiple
    # reconnects (if automatically reconnected)
    url = params["wamp_ws_url"]
    realm = params["wamp_realm"]

    session = MyAppSession(ComponentConfig(realm, {}))
    ssl_skip_verify = None
    if params["wamp_ssl_skip_verify"] == "True" :
        ssl_skip_verify = ssl.CertificateOptions(verify=False)

    runner = ApplicationRunner(
        url,
        ssl=ssl_skip_verify
    )

    runner.run(session, auto_reconnect=True)
