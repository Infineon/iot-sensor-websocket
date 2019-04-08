import json
import websocket
import time
 
ws = websocket.WebSocket()
ws.connect("ws://10.120.128.35")

JSON_API_Cmd = {
    "GET_DATA_ALL" : 0x01,
    "GET_DATA"     : 0x02
}

jsonGetData = {"command": JSON_API_Cmd["GET_DATA"], "sensors":["pressure","temperature"]}
jsonGetDataAll = {"command": JSON_API_Cmd["GET_DATA_ALL"]}

i = 0
nrOfMessages = 3
 
while i < nrOfMessages:
    ws.send(json.dumps(jsonGetDataAll))
    result = ws.recv()
    print(result)
    i = i + 1
    time.sleep(3)

i = 0
while i < nrOfMessages:
    ws.send(json.dumps(jsonGetData))
    result = ws.recv()
    print(result)
    i = i + 1
    time.sleep(3)

ws.close()

