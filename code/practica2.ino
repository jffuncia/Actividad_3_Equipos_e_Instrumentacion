
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
// Definición de constantes
// Pin y tipo del sensor DHT
#define DHTPIN 3
#define DHTTYPE DHT11
// Pin del Joystick
#define HORZ_PIN A1
#define VERT_PIN A2
#define SEL_PIN 7
// Pines del LED RGB
#define PIN_R 13
#define PIN_G 12
#define PIN_B 11
// Pin del sensor PIR
#define PIR_PIN 2
// Pin del boton
#define BUTTON_PIN 4
// Características del LDR
#define GAMMA 0.7f
#define RL10 50
// Umbrales de temperatura, humedad y luminosidad
#define TMP_MIN 25
#define TMP_MAX 28
#define HUM_MIN 30
#define HUM_MAX 50
#define LUX_MIN 30
#define LUX_MAX 200
//Pin del servo
#define servoPin 10
// Objeto del servo
Servo servo;
// variables para controlar el estado del sensor PIR
int pirState = LOW;
int motion = 0;
int pirEnabled = LOW;
// Variables para controlar la cuenta desde la última detección
float lastDetectionTime = 0;
float lastEnabledTime = 0;
//variables para el sensor de temperatura externo
float tempC=0; // Variable para almacenar el valor obtenido del sensor (0 a 1023)
int pinLM35 = 3; // Variable del pin de entrada del sensor (A0)
// Valor inicial del botón
int oldButtonValue = LOW;

// Menu inicial
int menu = 0;

// Consigna de temperatura
int consigna = 10;
//consigna luz
int consignaluz=10;
// Histeresis para abrir y cerrar la trampilla
const float histHisteresis = 1.0; // Histéresis para evitar oscilaciones rápidas
// Histeresis para iluminosidad
const float lumHisteresis = 1.0; // Histéresis para evitar oscilaciones rápidas
// Valor inicial del botón del joystick
int oldSelValue = LOW;
bool menuConsigna = false;

// Variables de control del rango de temperatura,
// humedad y luminosidad
bool isTemperatureOutOfRange = false;
bool isHumidityOutOfRange = false;
bool isLuxOutOfRange = false;

// Configuración del sesnor y LCD
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);


void setup() {
  // Inicialización del puerto serie
  Serial.begin(9600);

  // Inicialización del sensor DHT
  dht.begin();

  // Inicialización del LCD
  lcd.init();
  lcd.backlight();

  // Inicialización del servo
  servo.attach(servoPin);
  // Configuración de los pines como entrada o salida según corresponda
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(HORZ_PIN, INPUT);
  pinMode(VERT_PIN, INPUT);
  pinMode(SEL_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT);
}

void loop() {
  // Bloque del botón
  // Se lee el valor del botón y. si este ha cambiado con respecto 
  // al valor anterior y ahora está en alto (no pulsado), se considera
  // que ha finalizado la pulsación.
  // Lectura del segundo sensor de temperatura
  tempC = analogRead(pinLM35); 
  tempC = (5.0 * tempC * 100.0)/1024.0; 
  //Serial.println(tempC); 
  
  int newButtonValue = digitalRead(BUTTON_PIN);
  if (newButtonValue != oldButtonValue) {
    if (newButtonValue == HIGH) {
      pirEnabled = !pirEnabled; // Toggle del sensor PIR
      if (pirEnabled) {
        Serial.println("PIR enabled");
        lastEnabledTime = millis() / 1000; // guardamos el tiempo de activación
      } else {
        Serial.println("PIR disabled");
        lcd.setCursor(0, 1);
        lcd.print("                "); // limpiamos la cuenta anterior en el LCD para mostrar mensaje de desactivado
      }
    }
    oldButtonValue = newButtonValue; // Actualizamos el estado del botón
  }

  // Bloque del joystick
  int horz = analogRead(HORZ_PIN);
  if (horz >= 800) {
    menu = 0; // Establecer menú 0 si el joystick se ha movido a la izquierda
  } else if (horz <= 100) {
    menu = 1; // Establecer menú 1 si el joystick se ha movido a la derecha
  }

   //int vert = analogRead(VERT_PIN);

  // Bloque de lectura de temp, hum y lum
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  // LDR
  int analogValue = analogRead(A0);
  float voltage = analogValue / 1024.0 * 5.0;  // Conversión de la lectura analógica a voltaje
  float resistance = 1000.0 * voltage / (1.0 - voltage / 5.0); // Cálculo de la resistencia del LDR
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1.0 / GAMMA)); // Conversión de la resistencia a lux
  //Serial.println(analogRead(A0));

  // Bloque del sensor pIR
  if (pirEnabled) {
    motion = digitalRead(PIR_PIN);
    if (motion == HIGH && pirState == LOW) { // Si pasamos de no detección a detección (para no considerar dos detecciones cuando es solo una)
      lastDetectionTime = millis() / 1000; // Guardamos el instante de detección
      pirState = HIGH;
      Serial.println("Movimiento detectado");
      lcd.setCursor(0, 1);
      lcd.print("                "); // limpiamos la cuenta anterior en el LCD para iniciar la nueva
    } else if (motion == LOW && pirState == HIGH) { // Si pasamos de detección a no detección
      pirState = LOW;
      Serial.println("Movimiento finalizado!");
    }
  }


  // Bloque del LCD
  if (menu == 0) {
    //si se pulsa el boton del joystick cambiamos entre un menu que nos muestra la consigna de temperatura y otro que nos muestra los valores de temperatura y humedad.
    int newSelValue = digitalRead(SEL_PIN);
    if (newSelValue != oldSelValue) { 
      if (newSelValue == HIGH) {
        menuConsigna = !menuConsigna;
        lcd.clear();
      }
    }

    if (menuConsigna) {
      lcd.setCursor(0, 0);
      lcd.print("Consigna: ");
      int vert = analogRead(VERT_PIN);
      if (vert >= 800) {
        consigna++; // 
      } else if (vert <= 200) {
        consigna--; //
      }
      lcd.print(consigna); lcd.print("   ");
    } else {
      //mostramos la temperatura interior, la exterior y la humedad
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print(" ");
      lcd.print(tempC);
      lcd.setCursor(0, 1);
      lcd.print("Hum:  ");
      lcd.print(humidity);
      lcd.print("     ");
    }

  } else if (menu == 1) {
    //si se pulsa el boton del joystick cambiamos entre un menu que nos muestra la consigna de luz y otro que nos muestra los valores de luminosidad.
    int newSelValue = digitalRead(SEL_PIN);
    if (newSelValue != oldSelValue) { 
      if (newSelValue == HIGH) {
        menuConsigna = !menuConsigna;
        lcd.clear();
      }
    }

    if (menuConsigna) {
      lcd.setCursor(0, 0);
      lcd.print("Consigna: ");
      int vert = analogRead(VERT_PIN);
      if (vert >= 800) {
        consignaluz++; // 
      } else if (vert <= 200) {
        consignaluz--; //
      }
      lcd.print(consignaluz); lcd.print("   ");
    } else{
      lcd.setCursor(0, 0);
      lcd.print("Lum: ");
      lcd.print(lux);
      lcd.print("       ");
      lcd.setCursor(0, 1);
      if (pirEnabled) {
        // Hay dos situaciones en las que la cuenta debe iniciarse de nuevo (en 0s):
        // 1. Cuando se detecta un movimiento
        // 2. Cuando el PIR estaba desactivado y pasa a activado
        // Elegimos el instante de la situación más reciente y se lo restamos al tiempo actual
        // para obtener el tiempo transcurrido
        float elapsedTime = millis() / 1000 - max(lastDetectionTime, lastEnabledTime);
        lcd.print("Time detec: ");
        lcd.print(elapsedTime);
      } else {     
        lcd.print("PIR:  DEACT.");
      }
    }
  }
  //control iluminosidad
  if(lux > consignaluz + lumHisteresis){//si la luz es mayor que la consigna apagamos el led
    apagarled();
  }
  else if(lux < consignaluz - lumHisteresis){//si la luz es menor que la consigna encendemos el led
    encenderled();
  }
  else{
    //no hace nada
  }
  
  //control temperatura
  if(temperature>consigna + histHisteresis){//si la temperatura interna es mayor que la consigna
    if(temperature>tempC){//si la temperatura interna es mayor que la temperatura externa abrimos la trampilla
      abrirTrampilla();
      
    }
    else{//si la temperatura interna es menor que la temperatura externa cerramos la trampilla
      cerrarTrampilla();
    }

  }
  else if(temperature<consigna - histHisteresis){//si la temperatura interna es menor que la consigna
    if(temperature>tempC){//si la temperatura interna es mayor que la temperatura externa cerramos la trampilla
      cerrarTrampilla();
    }
    else{//si la temperatura interna es menor que la temperatura externa abrimos la trampilla
      abrirTrampilla();
    }
  }
  else {
    // No hacer nada
  }
  // Espera de 500 ms frecuencia de 0.5 Hz
  delay(500);
}
void abrirTrampilla() {
  // Mueve el servo para abrir la trampilla
  servo.write(0);
  
}
 
void cerrarTrampilla() {
  // Mueve el servo para cerrar la trampilla
  servo.write(90);
  
}
//ponemos el led  de color blanco
void encenderled(){
  analogWrite(PIN_R, 255);
  analogWrite(PIN_G,  255);
  analogWrite(PIN_B, 255);

}
//apagamos led
void apagarled(){
  analogWrite(PIN_R, 0);
  analogWrite(PIN_G,  0);
  analogWrite(PIN_B, 0);
}