
#include <OneWire.h>                  
#include <DallasTemperature.h>        
#include <Wire.h>                     
#include <LiquidCrystal_I2C.h>        
#include <L298N.h>
#include <HX711.h>    
#include <math.h>

LiquidCrystal_I2C lcd(0x27, 20,4);  // LCD i2c osoite

#define ONE_WIRE_BUS 8 // lämpöanturin kytkentäväylä
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

HX711 scale; // loadcell config

#define LOADCELL_DOUT_PIN 11 // loadcell kytkentä
#define LOADCELL_SCK_PIN 12

const int buttonPin1 = 2; // painonapit
const int buttonPin2 = 3;
const int buttonPin3 = 4;
const int buttonPin4 = 5;

const int buzzerPin = 6; // summeri

const int relayPin = 7; // keittolevyn virtakytkin

const int motorPin1 = 9; // moottoriohjaimen kytkentä
const int motorPin2 = 10;

const int motorSwitchPin = 13; // home switch ylhäällä

unsigned long previousMillis = 0;
const long interval = 20;

float amount;         // munien määrä
float doneness;       // keittoaste
float cooktime;     // keittoaika
float mass;    // munien massa
int cooktemp = 90;  // veden tavoitelämpötila
int state = 0;      // ohjelman vaihe

int button1;
int button2; 
int button3; 
int button4; 

int motorSwitch;

long bookmarktime;
long timeleft;

float calibration_factor = -7050.0; // vaa'an kalibrointikerroin

void setup() {
  Serial.begin(9600); // serial monitori ohjelmointia helpottamaan
  lcd.init(); 
  lcd.backlight();  
  
  pinMode(buttonPin1, INPUT_PULLUP); // painonappien pinmode
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
  pinMode(buttonPin4, INPUT_PULLUP);
  
  pinMode(motorSwitchPin, INPUT_PULLUP); // home kytkin pinmode
  
  pinMode(relayPin, OUTPUT); // rele pinmode
  
  digitalWrite(relayPin, HIGH);  // keittolevy aluksi pois päältä
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // loadcellin kytkentä ja kalibrointi
  scale.set_scale(calibration_factor);
  scale.tare();
  
  motor_init();  // siivilä ylös aluksi kunnes törmää mikrokytkimeen ja sitten alas vähän matkaa
  delay(100);
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  delay(2000);    
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
}

void loop() {
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
  
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    lcd.setCursor(12,3);
    lcd.print(temp);
    if (state == 0) {      
      menu1();
      state = 1;
      }
    else if (state == 1)  {
      mass = weigh_eggs();
      menu2();
      state = 3;
      }
    else if (state == 3)  {
      digitalWrite(relayPin, LOW); // levy päälle
      menu3();
      state = 4;
    }
    else if (state == 4 && temp >= cooktemp)  {
      menu4();
      motor(-1);  // laske munat veteen
      cooktime = cooktime_calc(doneness, amount, mass);
      state = 5;
    }
    else if (state == 5) {
      bookmarktime = millis()  +  60000 * cooktime;           // set the clock for 6 minutes in the future in milliseconds (1000 x 60 seconds x 6 minutes)
      state = 6;
    }
    else if (state == 6)  {                  
      timeleft = bookmarktime - millis();            // work out the time left on every cycle based on the actual time that's expired according to the Arduino internal clock
      lcd.setCursor(0,2);
      lcd.print("Time left:");
      lcd.setCursor(11,2);
      lcd.print(timeleft/(float)60000);
      if (timeleft <= 0) {state = 7;}
    }
    else if (state == 7)  {
      menu7();    
      motor(1);                        // munat pois vedestä
      tone(buzzerPin, 440, 1000);      // soita summeria
      digitalWrite(relayPin, HIGH);    // levy pois päältä
      menu_end();
      state = 0;
    }
  }
}

int menu1() {   // munien määrän valinta
  lcd.setCursor(0,0);
  lcd.print("------MUNAKONE------");
  lcd.setCursor(0,1);
  lcd.print("Laita munat koriin  ");
  lcd.setCursor(0,2);
  lcd.print("Kuinka monta munaa? ");
  lcd.setCursor(0,3);
  lcd.print("1, 2, 3, vai 4?     ");
  while(1){
    button1 = digitalRead(buttonPin1);
    button2 = digitalRead(buttonPin2);
    button3 = digitalRead(buttonPin3);
    button4 = digitalRead(buttonPin4);  
    if (button1 == LOW)  {amount = 1; return amount;}
    else if (button2 == LOW)  {amount = 2; return amount;}
    else if (button3 == LOW)  {amount = 3; return amount;}
    else if (button4 == LOW)  {amount = 4; return amount;}
  }
}

int menu2()  {    // keittoasteen valinta
  lcd.setCursor(0,0);
  lcd.print("Kuinka kovaksi?     ");
  lcd.setCursor(0,1);
  lcd.print("1=loysa, 2=pehmea   ");
  lcd.setCursor(0,2);
  lcd.print("3=kova    :DDD      ");
  lcd.setCursor(0,3);
  lcd.print("                    ");
  while(1){
    button1 = digitalRead(buttonPin1);
    button2 = digitalRead(buttonPin2);
    button3 = digitalRead(buttonPin3);
    if (button1 == LOW)  {doneness = 70; return doneness;}
    else if (button2 == LOW)  {doneness = 75; return doneness;}
    else if (button3 == LOW)  {doneness = 80; return doneness;}
  }
}

void menu3()  {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Vesi lampenee...");
  lcd.setCursor(0,3);
  lcd.print("Lampotila:");
  }

void menu4()  {
  lcd.setCursor(0,0);
  lcd.print("Vesi kiehuu!      ");  
}

void menu7()  {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Munat valmiita!");
}

void menu_end() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Aloita alusta");
  lcd.setCursor(0,1);
  lcd.print("painamalla nappia 1");
  while(1)  {
    button1 = digitalRead(buttonPin1);
    if (button1 == LOW) {return;}
  }
}

float weigh_eggs()  { // munien punnitseminen
  mass = scale.get_units();
  Serial.println(mass);
  return mass;
}

void motor(int dir) {     // nosta/laske siivilä
  if (dir == 1) {   // nosto
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
    delay(5000);
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);

    }
  if (dir == -1)  {   // lasku
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
    delay(5000);    
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
  }
}

bool pressed = false;
void motor_init()  {  
  while (not pressed) {
    motorSwitch = digitalRead(motorSwitchPin);
    if (motorSwitch == HIGH)  {
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);
      pressed = false;
    }
    else {pressed = true;}
  }
  return;
}

float cooktime_calc(float doneness, float amount, float mass)  {   // kaava munien keittoajan laskuun
  float t_egg = 21;
  float t_water = 95;  
  cooktime = 0.451 * (pow((mass/amount), (float)2/3)) * (log(0.76*((t_egg-t_water)/(doneness-t_water))));
  //cooktime = 0.5;    // testejä varten
  return cooktime;
}

