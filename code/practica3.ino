
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <Servo.h>
#include <IRremote.h>
#include <Fuzzy.h>
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
float servooutput;
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
int consigna = 25;
int consignaanterior=0;
//consigna luz
int consignaluz=10;
// Histeresis para abrir y cerrar la trampilla
const float histHisteresis = 1.0; // Histéresis para evitar oscilaciones rápidas
// Histeresis para iluminosidad
const float lumHisteresis = 1.0; // Histéresis para evitar oscilaciones rápidas
// Valor inicial del botón del joystick
int oldSelValue = LOW;
bool menuConsigna = false;
//definicion pin receptor ir
const int receiver = 6;
// Variables de control del rango de temperatura,
// humedad y luminosidad
bool isTemperatureOutOfRange = false;
bool isHumidityOutOfRange = false;
bool isLuxOutOfRange = false;
//se crea instancia fuzzy
Fuzzy* fuzzy = new Fuzzy();
// Configuración del sesnor y LCD
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
//Configuramos receprtor 
IRrecv irrecv(receiver);  // create instance of 'irrecv'
decode_results results;

void setup() {
  // Inicialización del puerto serie
  Serial.begin(9600);
  irrecv.enableIRIn(); // Inicia el receptor IR
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
  // Definir el conjunto difuso de entrada (temperatura)
  FuzzySet *veryCold = new FuzzySet(0, 0, 1, 2);
  FuzzySet *cold = new FuzzySet(1, 2, 3, 4);
  FuzzySet *warm = new FuzzySet(3, 4, 5, 6);
  FuzzySet *hot = new FuzzySet(5, 6, 7, 8);
  FuzzySet *veryHot = new FuzzySet(7, 8, 10, 10);

  // Añadir el conjunto de entrada a la lógica difusa
  FuzzyInput *temperature = new FuzzyInput(1);
  temperature->addFuzzySet(veryCold);
  temperature->addFuzzySet(cold);
  temperature->addFuzzySet(warm);
  temperature->addFuzzySet(hot);
  temperature->addFuzzySet(veryHot);
  fuzzy->addFuzzyInput(temperature);

  // Definir el conjunto difuso de salida (ángulo del servo)
  FuzzySet *servoAngle0 = new FuzzySet(0, 0, 18, 36);
  FuzzySet *servoAngle30 = new FuzzySet(18, 36, 54, 72);
  FuzzySet *servoAngle60 = new FuzzySet(54, 72, 90, 108);
  FuzzySet *servoAngle90 = new FuzzySet(90, 108, 126, 144);
  FuzzySet *servoAngle120 = new FuzzySet(126, 144, 162, 180);

  // Añadir el conjunto de salida a la lógica difusa
  FuzzyOutput *servoAngle = new FuzzyOutput(1);
  servoAngle->addFuzzySet(servoAngle0);
  servoAngle->addFuzzySet(servoAngle30);
  servoAngle->addFuzzySet(servoAngle60);
  servoAngle->addFuzzySet(servoAngle90);
  servoAngle->addFuzzySet(servoAngle120);
  fuzzy->addFuzzyOutput(servoAngle);

  // Definir las reglas difusas
  FuzzyRuleAntecedent *ifVeryCold = new FuzzyRuleAntecedent();
  ifVeryCold->joinSingle(veryCold);
  FuzzyRuleConsequent *thenServoAngle0 = new FuzzyRuleConsequent();
  thenServoAngle0->addOutput(servoAngle0);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, ifVeryCold, thenServoAngle0);
  fuzzy->addFuzzyRule(fuzzyRule1);

  FuzzyRuleAntecedent *ifCold = new FuzzyRuleAntecedent();
  ifCold->joinSingle(cold);
  FuzzyRuleConsequent *thenServoAngle30 = new FuzzyRuleConsequent();
  thenServoAngle30->addOutput(servoAngle30);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, ifCold, thenServoAngle30);
  fuzzy->addFuzzyRule(fuzzyRule2);

  FuzzyRuleAntecedent *ifWarm = new FuzzyRuleAntecedent();
  ifWarm->joinSingle(warm);
  FuzzyRuleConsequent *thenServoAngle60 = new FuzzyRuleConsequent();
  thenServoAngle60->addOutput(servoAngle60);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, ifWarm, thenServoAngle60);
  fuzzy->addFuzzyRule(fuzzyRule3);

  FuzzyRuleAntecedent *ifHot = new FuzzyRuleAntecedent();
  ifHot->joinSingle(hot);
  FuzzyRuleConsequent *thenServoAngle90 = new FuzzyRuleConsequent();
  thenServoAngle90->addOutput(servoAngle90);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, ifHot, thenServoAngle90);
  fuzzy->addFuzzyRule(fuzzyRule4);

  FuzzyRuleAntecedent *ifVeryHot = new FuzzyRuleAntecedent();
  ifVeryHot->joinSingle(veryHot);
  FuzzyRuleConsequent *thenServoAngle120 = new FuzzyRuleConsequent();
  thenServoAngle120->addOutput(servoAngle120);
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, ifVeryHot, thenServoAngle120);
  fuzzy->addFuzzyRule(fuzzyRule5);
}

void loop() {
 
  // Lectura del segundo sensor de temperatura
  tempC = analogRead(pinLM35); 
  tempC = (5.0 * tempC * 100.0)/1024.0; 
  //Serial.println(tempC); 
  // Bloque del botón
  // Se lee el valor del botón y. si este ha cambiado con respecto 
  // al valor anterior y ahora está en alto (no pulsado), se considera
  // que ha finalizado la pulsación.
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
 
  //Bloque del sensor de ir.
  if (irrecv.decode()){
    translateIR(); 
    irrecv.resume(); 
  }
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
    if(temperature>tempC){//si la temperatura interna es mayor que la temperatura externa abrimos la trampilla en función de el algoritmo inteligente fuzzy.
      //abrirTrampilla();
      if(consigna!=consignaanterior){
        fuzzy->setInput(1, (temperature-consigna));

      // Ejecuta el sistema difuso
        fuzzy->fuzzify();
    
      // Obtiene el resultado difuso
        servooutput = fuzzy->defuzzify(1);
        Serial.println(temperature-consigna);
        Serial.println(servooutput);
        servo.write(servooutput);
      }
      
    }
    else{//si la temperatura interna es menor que la temperatura externa cerramos la trampilla
      cerrarTrampilla();
    }

  }
  else if(temperature<consigna - histHisteresis){//si la temperatura interna es menor que la consigna
    if(temperature>tempC){//si la temperatura interna es mayor que la temperatura externa cerramos la trampilla
      cerrarTrampilla();
    }
    else{//si la temperatura interna es menor que la temperatura externa abrimos la trampilla en función de el algoritmo inteligente fuzzy.
      //abrirTrampilla();
      if(consigna!=consignaanterior){
        fuzzy->setInput(1, (consigna-temperature));

      // Ejecuta el sistema difuso
        fuzzy->fuzzify();
    
      // Obtiene el resultado difuso
        servooutput = fuzzy->defuzzify(1);
        Serial.println(consigna-temperature);
        Serial.println(servooutput);
        servo.write(servooutput);
      }
    }
  }
  else {
    // No hacer nada
  }
  // Espera de 500 ms frecuencia de 0.5 Hz
  consignaanterior=consigna;
  //Serial.println(servooutput);
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
//funcion mando
void translateIR()
{
    // takes action based on IR code received
    // describing Remote IR codes
    switch(irrecv.decodedIRData.command)
    {
        case 162: Serial.println("POWER"); break;
        case 67: if(menu==0){
          menu=1;
        }; break;
        case 64: menuConsigna=!menuConsigna;
         lcd.clear();
         break;
        case 68: if(menu==1){
          menu=0;
        }; break;
        case 21: if((menu==0) && (menuConsigna)){
          consigna--;
        }
        else if((menu==1)&&(menuConsigna)){
          consignaluz--;
        }; break;
        case 70: if((menu==0) && (menuConsigna)){
          consigna++;
        }
        else if((menu==1)&&(menuConsigna)){
          consignaluz++;
        }; break;

        case 0xFFFFFFFF: Serial.println("REPEAT");break;  
        default: 
            Serial.println(irrecv.decodedIRData.command);
    }
} 