#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 2      // Pin donde está conectado el DHT22
#define DHTTYPE DHT22 // Tipo de sensor

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200); // Configurar comunicación serie
  dht.begin();          // Iniciar el sensor DHT22
}

void loop() {
  float temp = dht.readTemperature(); // Leer temperatura
  float hum = dht.readHumidity();     // Leer humedad

  if (!isnan(temp) && !isnan(hum)) {  // Verificar que los valores sean válidos
    Serial.print("TEMP:");
    Serial.print(temp);
    Serial.print(",HUM:");
    Serial.println(hum);
  } else {
    Serial.println("Error al leer el sensor");
  }

  delay(2000); // Esperar 2 segundos antes de la próxima lectura
}
