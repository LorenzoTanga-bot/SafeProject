from . import snapshot
import json


class TextWAMP:

    def __init__(self):
        self.args = {}
        self.params = {}
        self.builder = snapshot.SnapshotBuilder()

    def setM(self, k, v):
        if k is not None:
            self.args[k] = v

    def setV(self, k, v):
        if k is not None:
            self.params[k] = v

    def getSnapshot(self, deviceId, sourceNode, destinationNode):
        sn = self.builder.make_snapshot("jzp://edv#" + deviceId + ".0000")
        for k, v in self.args.items():
            self.builder.add_measure(k, v, sn, sourceNode, destinationNode)
        return sn
