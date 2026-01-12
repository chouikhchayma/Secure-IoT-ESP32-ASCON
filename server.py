from flask import Flask, render_template
from flask_socketio import SocketIO
import json
import paho.mqtt.client as mqtt
import ascon

# --- Configuration ---
MQTT_BROKER = "192.168.141.56"  # ton broker MQTT
MQTT_PORT = 1883
MQTT_TOPIC = "iot/soil/humidity"
MQTT_USER = "esp32user"
MQTT_PASS = "chayma2002"

# Clé partagée (doit être la même que dans l'ESP32)
KEY = bytes.fromhex("000102030405060708090A0B0C0D0E0F")

# --- Flask setup ---
app = Flask(__name__)
socketio = SocketIO(app)

# --- MQTT callbacks ---
def on_connect(client, userdata, flags, rc):
    print("✅ Connected to MQTT broker")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        iv = bytes.fromhex(data["iv"])
        cipher = bytes.fromhex(data["cipher"])
        tag = bytes.fromhex(data["tag"])
        
        # Déchiffrement avec ASCON-128
        plaintext = ascon.decrypt(
            key=KEY,
            nonce=iv,
            associateddata=b"",
            ciphertext=cipher + tag,
            variant="Ascon-128"
        )
        humidity_value = plaintext.decode()
        print("💧 Humidity:", humidity_value)

        # Envoie au client web via SocketIO
        socketio.emit('humidity_update', {'humidity': humidity_value})

    except Exception as e:
        print("❌ Failed to decrypt message:", e)

# --- MQTT client setup ---
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASS)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

# --- Flask route ---
@app.route('/')
def index():
    return render_template('index.html')

# --- Main ---
if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
