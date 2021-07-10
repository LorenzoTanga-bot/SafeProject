import logging
import json
import uuid
from datetime import datetime
import time
import math


class SnapshotBuilder(object):

    def make_snapshot(self, ref):
        _dt = datetime.now()
        dt = _dt.replace(microsecond=0).isoformat()
        data = {}
        data['ref'] = ref
        data['tz'] = (str(dt) + ".000+01:00")
        data['type'] = "presence_uwb"
        data['uuid'] = str(uuid.uuid4())
        data['m'] = []
        return data

    def add_measure(self, k, v, sn):
        m = {}
        m['k'] = k
        m['tz'] = sn["tz"]
        m['v'] = v
        sn['m'].append(m)