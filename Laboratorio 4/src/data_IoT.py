import paho.mqtt.client as mqtt
import json
import time
import serial
import sys

# Puerto serie desde los argumentos de la línea de comandos
PORT = sys.argv[1]

# Configuración del broker MQTT
BROKER = "iot.eie.ucr.ac.cr"
PORT_MQTT = 1883
TOPIC_TELEMETRY = "v1/devices/me/telemetry"
TOPIC_REQUEST = "v1/devices/me/attributes/request/1/"
USERNAME = "mJfQiM1Tqf7fAXMfULNE"

# Diccionario para almacenar los datos 
data_dict = {'temp': 0, 'x': 0, 'y': 0, 'z': 0, 'bat': 0, 'low_bat': "", "deg_alert": ""}

def on_connect(client, userdata, flags, rc):
    """Cliente se conecta al broker."""
    print(f"Conectado con código de resultado {rc}")
    client.subscribe(TOPIC_REQUEST)

def on_message(client, userdata, msg):
    """Se recibe un mensaje en un tópico suscrito."""
    print(f"Mensaje recibido en {msg.topic}: {msg.payload.decode()}")

def setup_mqtt_client():
    """Configura y devuelve un cliente MQTT listo para usarse."""
    client = mqtt.Client()
    client.username_pw_set(USERNAME)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, PORT_MQTT, 60)
    client.loop_start()
    return client

def setup_serial_connection(port):
    """Configura y devuelve una conexión serial con el puerto STM."""
    try:
        return serial.Serial(port, 115200, timeout=1)
    except serial.SerialException as e:
        print(f"Error al abrir el puerto serial: {e}")
        sys.exit(1)

def parse_sensor_data(data):
    """Compara los datos recibidos del puerto serie y devuelve un diccionario con los valores."""
    try:
        dsplit = data.strip().split()
        if len(dsplit) != 5:
            return None

        x, y, z = map(float, dsplit[:3])
        temp = int(dsplit[3])
        batt = float(dsplit[4]) * 100 / 9
        deg_alert = any(abs(axis) > 5 for axis in (x, y, z))

        return {
            'x': x,
            'y': y,
            'z': z,
            'temp': temp,
            'bat': batt,
            'low_bat': "Full Battery" if batt >= 90 else "Low Battery" if batt < 80 else "Medium Battery",
            'deg_alert': "Danger!" if deg_alert else "All good."
        }
    except (ValueError, IndexError):
        return None

def main():
    client = setup_mqtt_client()
    ser = setup_serial_connection(PORT)
    
    try:
        while True:
            if ser.in_waiting > 0:
                raw_data = ser.readline().decode('utf-8', errors='ignore')
                print(f"Datos crudos recibidos: {raw_data}")

                sensor_data = parse_sensor_data(raw_data)
                
                if sensor_data:
                    client.publish(TOPIC_TELEMETRY, json.dumps(sensor_data))
                    print(f"Datos enviados: {sensor_data}")
                
                time.sleep(0.1)  # Pequeña pausa para evitar sobrecarga
    
    except KeyboardInterrupt:
        print("\nSaliendo del programa.")
    
    finally:
        ser.close()
        client.loop_stop()
        client.disconnect()
        print("Conexión serial cerrada y desconectado del broker MQTT.")

if __name__ == "__main__":
    main()
