import requests, json
import time
from influxdb import InfluxDBClient
from datetime import datetime

client = InfluxDBClient('192.168.1.67', 8086, '', '', 'weather')

while (1):
    r = requests.get("http://192.168.1.91")
    data = r.json()
    print(json.dumps(data))

    temp = [
        {
            "measurement": "temp",
            "tags": {
            },
            "time": datetime.utcfromtimestamp(time.time()),
            "fields": {
                "value": data['temp']
            }
        }
    ]

    pres = [
        {
            "measurement": "pres",
            "tags": {
            },
            "time": datetime.utcfromtimestamp(time.time()),
            "fields": {
                "value": data['pres']
            }
        }
    ]

    hum = [
        {
            "measurement": "hum",
            "tags": {
            },
            "time": datetime.utcfromtimestamp(time.time()),
            "fields": {
                "value": data['hum']
            }
        }
    ]


    client.write_points(temp)
    client.write_points(pres)
    client.write_points(hum)
    time.sleep(60)
