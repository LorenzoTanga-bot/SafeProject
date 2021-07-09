FROM python:3.7.7
WORKDIR /data/mqtt-wamp-bridge

ADD requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

ADD . ./

CMD [ "python", "./app.py" ]
