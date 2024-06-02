import websocket, rel, json, struct

DEVICE_IP_ADDRESS = "192.168.1.23"

current_time_offset_ms = 0
last_time_stamp_ms = 0

def chunk_string(string, length):
    return (string[0+i:length+i] for i in range(0, len(string), length))

def decode_message(message):
    global last_time_stamp_ms
    global current_time_offset_ms

    # Message is JSON
    message_dict = json.loads(message)
    # Iterate over buses and devices
    for bus in message_dict:
        for device in message_dict[bus]:
            # Get the hex message data (may contain multiple samples)
            hex_data = message_dict[bus][device]["x"]
            # Split based on message length
            hex_samples_and_ts = chunk_string(hex_data, 28)
            for hex_sample_and_ts in hex_samples_and_ts:
                # Bytes from hex
                bytes_sample_and_ts = bytes.fromhex(hex_sample_and_ts)
                # Extract timestamp using struct
                time_stamp_wrapped_ms = struct.unpack(">H", bytes_sample_and_ts[:2])[0]
                # Unwrap the timestamp
                if time_stamp_wrapped_ms < last_time_stamp_ms:
                    current_time_offset_ms += 65536
                last_time_stamp_ms = time_stamp_wrapped_ms
                time_stamp_ms = current_time_offset_ms + time_stamp_wrapped_ms
                # Extract the samples using struct
                gyro_x, gyro_y, gyro_z, acc_x, acc_y, acc_z = struct.unpack("<hhhhhh", bytes_sample_and_ts[2:])
                # Form the decoded string
                decoded_string = f"Gyro(dps): ({gyro_x/16.384:.2f}, {gyro_y/16.384:.2f}, {gyro_z/16.384:.2f}), Acc(g): ({acc_x/8192:.2f}, {acc_y/8192:.2f}, {acc_z/8192:.2f})"
                print(f"Time: {time_stamp_ms}, {decoded_string}")
    
def on_message(ws, message):
    decode_message(message)

def on_error(ws, error):
    print(error)

def on_close(ws, close_status_code, close_msg):
    print("### closed ###")

def on_open(ws):
    # Subscribe to messages
    subscribe_topic = "IMU"
    subscribe_cmd = {
        "cmdName": "subscription",
        "action": "update",
        "pubRecs": [
            {"name": subscribe_topic, "msgID": subscribe_topic, "trigger": "timeorchange", "rateHz": 0.1},
        ]
    }
    print("Opened connection - subscribing to messages with ", json.dumps(subscribe_cmd))
    ws.send(json.dumps(subscribe_cmd))

if __name__ == "__main__":
    # websocket.enableTrace(True)
    ws = websocket.WebSocketApp("ws://" + DEVICE_IP_ADDRESS + "/wsjson",
                on_open=on_open, on_message=on_message, on_error=on_error, on_close=on_close)
    ws.run_forever(dispatcher=rel, reconnect=5)
    rel.signal(2, rel.abort)
    rel.dispatch()