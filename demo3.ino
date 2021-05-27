
#include <OneWire.h>                  
#include <DallasTemperature.h>        
#include <Wire.h>                     
#include <LiquidCrystal_I2C.h>        
#include <L298N.h>
#include <HX711.h>    
#include <math.h>

LiquidCrystal_I2C lcd(0x27, 20,4);  // Set the LCD I2C address

#define ONE_WIRE_BUS 8
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

#define LOADCELL_DOUT_PIN 11
#define LOADCELL_SCK_PIN 12

int buttonPin1 = 2;
int buttonPin2 = 3;
int buttonPin3 = 4;
int buttonPin4 = 5;

unsigned long previousMillis = 0;
const long interval = 20;

const int motorPin1 = 9;
const int motorPin2 = 10;

const int relayPin = 7;

int amount;         // munien määrä
int doneness;       // keittoaste
float cooktime;     // keittoaika
int state = 0;      // ohjelman vaihe
float mass = 58;    // munien massa
int cooktemp = 26;  // haluttu lämpötila keittämiselle

int button1;
int button2; 
int button3; 
int button4; 

long bookmarktime;
long timeleft;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();  
  
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
  pinMode(buttonPin4, INPUT_PULLUP);
  
  pinMode(relayPin, OUTPUT);
  
  L298N motorA(motorPin1, motorPin2);

}

void loop() {
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {  // start timed event for read and send
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
      menu2();
      state = 3;
      }
    else if (state == 3)  {
      digitalWrite(relayPin, HIGH); // levy päälle
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
      lcd.print(timeleft);
      if (timeleft <= 0) {state = 7;}
    }
    else if (state == 7)  {
      menu7();
      motor(1);                       // munat pois vedestä
      digitalWrite(relayPin, LOW);    // levy pois päältä
      menu_end();
      state = 0;
    }
  }
}

int menu1() {   // munien määrän valinta
  lcd.setCursor(0,0);
  lcd.print("------MUNAKONE------");
  lcd.setCursor(0,1);
  lcd.print("Montako munaa?      ");
  lcd.setCursor(0,2);
  lcd.print("1, 2, 3, vai 4?     ");
  lcd.setCursor(0,3);
  lcd.print("                    ");
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
  lcd.setCursor(16,0);
  lcd.print(amount);
  lcd.setCursor(0,1);
  lcd.print("1=loysa, 2=pehmea");
  lcd.setCursor(0,2);
  lcd.print("3=kova    :DDD     ");
  lcd.setCursor(0,3);
  lcd.print("                    ");
  while(1){
    button1 = digitalRead(buttonPin1);
    button2 = digitalRead(buttonPin2);
    button3 = digitalRead(buttonPin3);
    if (button1 == LOW)  {doneness = 60; return doneness;}
    else if (button2 == LOW)  {doneness = 65; return doneness;}
    else if (button3 == LOW)  {doneness = 70; return doneness;}
  }
}

void menu3()  {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Vesi lampenee!");
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

int show_temp()  {    //  näytä lämpötila (ei tarvita)
  sensors.requestTemperatures(); //
  float temp = sensors.getTempCByIndex(0);
  lcd.setCursor(11,1);
  lcd.print(temp);
  }


void motor(int dir) {     // nosta/laske siivilä
  if (dir == 1) {   // nosto
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
    delay(3000);
    }
  if (dir == -1)  {   // lasku
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
    delay(3000);
  }
}

float cooktime_calc(int doneness, int amount, float mass)  {   // kaava munien keittoajan laskuun
  int t_egg = 21;
  int t_water = 95;  
  cooktime = 0.451 * (pow((mass/amount), (float)(2/3))) * (log(0.76*((t_egg-t_water)/(doneness-t_water))));
  Serial.println(cooktime);
  return cooktime;
}

