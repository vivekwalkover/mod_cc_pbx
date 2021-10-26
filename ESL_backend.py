import ESL
import requests as reqs
import json
import mysql.connector
from mysql.connector import Error
import logging
import datetime
from mysql.connector import Error
from mysql.connector.connection import MySQLConnection
from mysql.connector import pooling
import logging
import threading
import schedule
import time
import datetime
import time

with open('/var/www/html/fs_backend/config.json') as f:
    data = json.load(f)

logging.basicConfig(filename="/root/esl.log",
                    format='%(asctime)s:%(levelname)s:%(messege)s'
                    )
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

while is True:
    
     time.sleep(0.00000000005)

def get_loc(ip):
    url = "http://ipwhois.app/json/" + ip
    response = reqs.get(url)
    # print(response,response.text.longitude)
    resp = json.loads(response.text)
    # print(resp,resp["longitude"])
    return [resp["latitude"], resp["longitude"]]


# Connect to FreeSWITCH using ESL
def profile_info(query):
    try:
        connection = mysql.connector.connect(host='119.18.55.154',
                                             database='cc_master',
                                             user='ccuser',
