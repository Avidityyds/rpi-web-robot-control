"""
RPi MQTT → Serial 控制 Arduino
訂閱單一字串化 JSON payload：{"L":-255..255, "R":-255..255, "vacuum": true|false}
轉成 CSV 串口一行送出：L,R,V\n  (V=1/0)
"""

import json, time, sys
import paho.mqtt.client as mqtt
import serial

# ======== 可調參數 ========
MQTT_HOST   = "localhost"
MQTT_PORT   = 1883
MQTT_TOPIC  = "robot/motor_pwm"     # 與前端搖桿相同
SERIAL_PORT = "/dev/ttyACM0"        # 依接線調整
SERIAL_BAUD = 115200
SERIAL_TIMEOUT = 0.1                # 讀取超時
# =========================

ser = None

def open_serial():
    global ser
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=SERIAL_TIMEOUT)
            print(f"[SERIAL] Opened {SERIAL_PORT} @ {SERIAL_BAUD}")
            return
        except Exception as e:
            print(f"[SERIAL] Open failed: {e}. Retry in 2s...")
            time.sleep(2)

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[MQTT] Connected. Subscribing {MQTT_TOPIC}")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"[MQTT] Connect failed rc={rc}")

def sanitize_pwm(x):
    # 允許 -255..255 的整數
    try:
        xi = int(x)
    except:
        xi = 0
    if xi > 255:  xi = 255
    if xi < -255: xi = -255
    return xi

def on_message(client, userdata, msg):
    global ser
    try:
        payload = msg.payload.decode('utf-8', errors='ignore')
        obj = json.loads(payload)
        L = sanitize_pwm(obj.get("L", 0))
        R = sanitize_pwm(obj.get("R", 0))
        V = 1 if bool(obj.get("vacuum", False)) else 0
        line = f"{L},{R},{V}\n"
        # 串口可能斷線，嘗試重開
        if (ser is None) or (not ser.is_open):
            open_serial()
        ser.write(line.encode('utf-8'))
        # 可選：列印觀察
        print(f"[SERIAL ⇐ MQTT] {line.strip()}")
    except Exception as e:
        print(f"[ERROR] {e} | raw={msg.payload}")

def main():
    open_serial()
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    # 若有帳密在此 set_username_pw
    # client.username_pw_set("user","pass")
    client.connect(MQTT_HOST, MQTT_PORT, keepalive=30)
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        pass
    finally:
        if ser and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()
