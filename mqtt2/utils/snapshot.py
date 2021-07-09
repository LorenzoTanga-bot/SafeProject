import logging
import json
import uuid
from datetime import datetime
import time
import math


class SnapshotBuilder(object):

    def make_snapshot(self, ref, type):
        _dt = datetime.now()
        dt = _dt.replace(microsecond=0).isoformat()
        data = {}
        data['ref'] = ref
        # data['t'] = int(time.mktime(_dt.timetuple()) * 1e3 + _dt.microsecond / 1e3)
        data['tz'] = (str(dt) + ".000+01:00")
        # data['cat'] = "uwb_distance"
        data['type'] = type
        data['uuid'] = str(uuid.uuid4())
        data['m'] = []
        # data['r'] = []
        return data

    def add_measure(self, k, v, sn):
        m = {}
        # m['d'] = destinationNode
        # m['s'] = sourceNode
        m['k'] = k
        # m['t'] = sn["t"]
        m['tz'] = sn["tz"]
        m['v'] = v
        # m['u'] = ""
        # m['x'] = "rank:1"
        # m['rank'] = 1
        sn['m'].append(m)
