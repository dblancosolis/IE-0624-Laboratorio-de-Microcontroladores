import paho.mqtt.client as mqtt
import json
import time
import serial
import sys

# Puerto Serial (ajustar según el sistema operativo)
PORT = "/dev/ttyACM0"  # Linux/Mac (en Windows usa "COMX")

# Configuración del broker MQTT (ThingsBoard)
BROKER = "iot.eie.ucr.ac.cr" # Servidor MQTT de ThingsBoard
PORT_MQTT = 1883
TOPIC_TELEMETRY = "v1/devices/me/telemetry"
USERNAME = "mJfQiM1Tqf7fAXMfULNE"  # Access Token de ThingsBoard

# Función al conectarse al broker
def on_connect(client, userdata, flags, rc):
    print(f"Conectado con código {rc}")
    client.subscribe(TOPIC_TELEMETRY)

# Configurar MQTT
def setup_mqtt_client():
    client = mqtt.Client()
    client.username_pw_set(USERNAME)
    client.on_connect = on_connect
    client.connect(BROKER, PORT_MQTT, 60)
    client.loop_start()
    return client

# Configurar conexión Serial con Arduino
def setup_serial_connection(port):
    try:
        return serial.Serial(port, 115200, timeout=1)
    except serial.SerialException as e:
        print(f"Error al abrir el puerto serial: {e}")
        sys.exit(1)

# Leer y procesar datos del Arduino
def main():
    client = setup_mqtt_client()
    ser = setup_serial_connection(PORT)
    
    try:
        while True:
            if ser.in_waiting > 0:
                raw_data = ser.readline().decode('utf-8', errors='ignore').strip()
                print(f"Datos recibidos: {raw_data}")

                # Separar los datos de temperatura y humedad
                try:
                    temp, hum = map(float, raw_data.split())
                    payload = {'temperature': temp, 'humidity': hum}
                    
                    # Enviar datos a ThingsBoard
                    client.publish(TOPIC_TELEMETRY, json.dumps(payload))
                    print(f"Datos enviados: {payload}")

                except ValueError:
                    print("Error: Datos inválidos recibidos")

                time.sleep(2)  # Esperar 2 segundos entre lecturas
    
    except KeyboardInterrupt:
        print("\nSaliendo del programa.")

    finally:
        ser.close()
        client.loop_stop()
        client.disconnect()
        print("Conexión serial cerrada y desconectado del broker MQTT.")

if __name__ == "__main__":
    main()
