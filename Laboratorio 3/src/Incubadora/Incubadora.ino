#include <PCD8544.h>
#include <PID_v1_bc.h>

// Pantalla LCD
PCD8544 lcd;

// Variables para el PID
double Input, Output, Setpoint;
PID myPID(&Input, &Output, &Setpoint, 15, 0.5, 1, DIRECT);

// Pines y configuración inicial
const int LED_LOW = 8, LED_MED = 12, LED_HIGH = 13;
const int SENSOR_TEMP = A0, SENSOR_LCD = A1, SENSOR_SERIAL = A2;

void setup() {
  Serial.begin(9600);
  pinMode(LED_LOW, OUTPUT);
  pinMode(LED_MED, OUTPUT);
  pinMode(LED_HIGH, OUTPUT);
  lcd.begin();
  myPID.SetOutputLimits(-125, 125);
  myPID.SetMode(AUTOMATIC);
}

void loop() {
  leerSensores();
  myPID.Compute();
  actualizarLCD();
  actualizarLEDs();
  enviarDatosSerial();
  delay(500);
}

void leerSensores() {
  Setpoint = map(analogRead(SENSOR_TEMP), 0, 1023, 20, 80);
  float tempWatts = (Output * 25.0) / 255.0;
  Input = simPlant(tempWatts);
}

void actualizarLCD() {
  lcd.setPower(analogRead(SENSOR_LCD) > 700);
  lcd.setCursor(0, 1); lcd.print("Opt: "); lcd.print(Setpoint);
  lcd.setCursor(0, 2); lcd.print("Ctrl: "); lcd.print(Output);
  lcd.setCursor(0, 3); lcd.print("Temp: "); lcd.print(Input);
}

void actualizarLEDs() {
  digitalWrite(LED_LOW, Input < 30);
  digitalWrite(LED_MED, Input >= 30 && Input <= 42);
  digitalWrite(LED_HIGH, Input > 42);
}

void enviarDatosSerial() {
  if (analogRead(SENSOR_SERIAL) > 700) {
    Serial.print(Setpoint); Serial.print(",");
    Serial.print(Output); Serial.print(",");
    Serial.println(Input);
  }
}

// Respuesta térmica de la planta
float simPlant(float Q) { // heat input in W (or J/s)
  // simulate a 1x1x2cm aluminum block with a heater and passive ambient cooling
 // float C = 237; // W/mK thermal conduction coefficient for Al
  float h = 5; // W/m2K thermal convection coefficient for Al passive
  float Cps = 0.89; // J/g°C
  float area = 1e-4; // m2 area for convection
  float mass = 10; // g
  float Tamb = 25; // °C
  static float T = Tamb; // °C
  static uint32_t last = 0;
  uint32_t interval = 100; // ms

  if (millis() - last >= interval) {
    last += interval;
    // 0-dimensional heat transfer
    T = T + Q * interval / 1000 / mass / Cps - (T - Tamb) * area * h;
  }
  return T;
}