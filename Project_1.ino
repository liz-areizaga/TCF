#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT11

#define echo 13
#define trigger 12
#define led 8
#define fan_in 4
#define speaker 6
#define button 10

DHT dht(DHTPIN, DHTTYPE);

//global variables for timing
unsigned long DetectDistance_ElapsedTime; 
unsigned long DetectDistance_period = 500;
unsigned long Alarm_ElapsedTime;
unsigned long Alarm_period = 200;
unsigned long SensorLight_ElapsedTime;
unsigned long SensorLight_period = 500;
unsigned long DetectTemp_ElapsedTime;
unsigned long DetectTemp_period = 4000;
unsigned long SystemCond_ElapsedTime;
unsigned long SystemCond_period = 200;
unsigned long currentMillis;

//global variables for states
float f; //temperature
int fan = 1;
int distance = 0;
long travelTime = 0;
int sensor_light = 0;
int alarm = 0;
int on = 0;

//function declarations
void getDistance();
void getTemperature();

enum SystemCond_States { SystemCond_Start, sys_off_r, sys_on_p, sys_on_r, sys_off_p} SystemCond_state;
void SystemCond_TickFunc(){
  switch(SystemCond_state){
    case SystemCond_Start:
    SystemCond_state = sys_off_r;
    on = 0;
    break;

    case sys_off_r:
    if(digitalRead(button) == HIGH){
      SystemCond_state = sys_on_p;
    } else {
      SystemCond_state = sys_off_r;
    }
    break;

    case sys_on_p:
    if(digitalRead(button) == HIGH){
      SystemCond_state = sys_on_p;
    } else {
      SystemCond_state = sys_on_r;
    }
    break;

    case sys_on_r:
    if(digitalRead(button) == HIGH){
      SystemCond_state = sys_off_p;
    } else {
      SystemCond_state = sys_on_r;
    }
    break;

    case sys_off_p:
    if(digitalRead(button) == HIGH){
      SystemCond_state = sys_off_p;
    } else {
      SystemCond_state = sys_off_r;
    }
    break;

    default:
    SystemCond_state = SystemCond_Start;
    break;
  }

  switch(SystemCond_state){
    case SystemCond_Start:
    break;

    case sys_off_r:
    break;

    case sys_on_p:
    on = 1;
    break;

    case sys_on_r:
    break;

    case sys_off_p:
    on = 0;
    break;

    default: break;
  }
}

enum DetectDistance_States {DetectDistance_Start, off_sensor, safe, not_safe} DetectDistance_state;
void DetectDistance_TickFunc(){
  getDistance();
  Serial.println(distance);
  switch (DetectDistance_state){
    case DetectDistance_Start:
    DetectDistance_state = off_sensor;
    sensor_light = 0;
    alarm = 0;
    break;

    case off_sensor:
    if(fan == 0){
      DetectDistance_state = off_sensor;
    } else {
      DetectDistance_state = safe;
    }
    break;

    case safe:
    if(distance > 5 && fan == 1){
      DetectDistance_state = safe;
    } else if (distance <= 5 && fan == 1){
      DetectDistance_state = not_safe;
    } else if ( fan == 0){
      DetectDistance_state = off_sensor;
    }
    break;

    case not_safe:
    if(distance <= 5 && fan == 1){
      DetectDistance_state = not_safe;
    } else if (distance > 5 && fan == 1){
      DetectDistance_state = safe;
    } else if(fan == 0){
      DetectDistance_state = off_sensor;
    }
    break;

    default:
    DetectDistance_state = DetectDistance_Start;
    break;
  }

  switch(DetectDistance_state){
    case DetectDistance_Start:
    break;

    case off_sensor:
    sensor_light = 0;
    alarm = 0;
    break;

    case safe:
    sensor_light = 0;
    alarm = 0;
    break;

    case not_safe:
    sensor_light = 1;
    alarm = 1;
    break;

    default: break;
    
  }
}

enum SensorLight_SM {SensorLight_Start, SensorLight_off, SensorLight_on} SensorLight_state;
void SensorLight_TickFunc(){
  switch (SensorLight_state){
    case SensorLight_Start:
    digitalWrite(led, LOW);
    SensorLight_state = SensorLight_off;
    break;

    case SensorLight_off:
    if(sensor_light == 0){
      SensorLight_state = SensorLight_off;
    } else {
      SensorLight_state = SensorLight_on;
    }
    break;

    case SensorLight_on:
    if(sensor_light == 1){
      SensorLight_state = SensorLight_on;
    } else {
      SensorLight_state = SensorLight_off;
    }
    break;
    

    default:
    SensorLight_state = SensorLight_Start;
    break;
  }

  switch(SensorLight_state){
    case SensorLight_Start:
    break;

    case SensorLight_off:
    digitalWrite(led, LOW);
    break;

    case SensorLight_on:
    digitalWrite(led, !digitalRead(led));
    break;

    default:break;
  }
}

enum Alarm_States{ Alarm_Start, Alarm_off, Alarm_on, Alarm_low} Alarm_state;
void Alarm_TickFunc(){
  switch(Alarm_state){
    case Alarm_Start:
    Alarm_state = Alarm_off;
    break;

    case Alarm_off:
    if(alarm == 0){
      Alarm_state = Alarm_off;
    }else {
      Alarm_state = Alarm_on;
    }
    break;

    case Alarm_on:
    if(alarm == 0){
      Alarm_state = Alarm_off;
    }else {
      Alarm_state = Alarm_on;
    }
    break;

    default:
    Alarm_state = Alarm_Start;
    break;
    
  }

  switch(Alarm_state){
    case Alarm_Start:
    break;

    case Alarm_off:
    digitalWrite(speaker,LOW);
    break;

    case Alarm_on:
    digitalWrite(speaker, HIGH);
    break;

    default:break;
  }
}

enum DetectTemp_States { DetectTemp_Start, no_temp, reg_temp, hot_temp} DetectTemp_state;
void DetectTemp_TickFunc(){
  getTemperature();
  switch(DetectTemp_state){
    
    case DetectTemp_Start:
    DetectTemp_state = no_temp;
    fan = 0;
    break;

    case no_temp:
    if(on == 0){
      DetectTemp_state = no_temp;
    } else {
      DetectTemp_state = reg_temp;
    }
    break;

    case reg_temp:
    if(f < 80.0 && on == 1){
      DetectTemp_state = reg_temp;
    } else if(f >= 80.0 && on == 1) {
      DetectTemp_state = hot_temp;
    } else if(on == 0){
      DetectTemp_state = no_temp;
    }
    break;

    case hot_temp:
    if(f < 80.0 && on == 1){
      DetectTemp_state = reg_temp;
    } else if (f >= 80.0 && on == 1){
      DetectTemp_state = hot_temp;
    } else if(on == 0){
      DetectTemp_state = no_temp;
    }
    break;

    default:
    DetectTemp_state = DetectTemp_Start;
    break;
  }

  switch(DetectTemp_state){
    case DetectTemp_Start:
    break;

    case no_temp:
    digitalWrite(fan_in, LOW);
    break;

    case reg_temp:
    digitalWrite(fan_in, LOW); 
    fan = 0;
    break;

    case hot_temp:
    digitalWrite(fan_in, HIGH);
    fan = 1;
    break;

    default: break;
    
  }
}
  
void setup() {
  // put your setup code here, to run once:
  pinMode(echo, INPUT);
  pinMode(trigger, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(fan_in, OUTPUT);
  pinMode(speaker, OUTPUT);
  pinMode(button, INPUT);
  
  Serial.begin(9600);// for output
  
  dht.begin(); //to start temperature sensor

  //state initializing
  DetectDistance_state = DetectDistance_Start;
  SensorLight_state = SensorLight_Start;
  Alarm_state = Alarm_Start;
  DetectTemp_state = DetectTemp_Start;
  SystemCond_state = SystemCond_Start;

  //initalize elapsedTime
  DetectDistance_ElapsedTime = millis();
  SensorLight_ElapsedTime = millis();
  Alarm_ElapsedTime = millis();
  DetectTemp_ElapsedTime = millis();
  SystemCond_ElapsedTime = millis();
}



void loop() {
  currentMillis = millis();
  if ( currentMillis - SystemCond_ElapsedTime >= SystemCond_period){
    SystemCond_ElapsedTime = currentMillis;
    SystemCond_TickFunc();
    //Serial.print("Running DetectDistance\n");
  }  
  if ( currentMillis - DetectDistance_ElapsedTime >= DetectDistance_period){
    DetectDistance_ElapsedTime = currentMillis;
    DetectDistance_TickFunc();
    //Serial.print("Running DetectDistance\n");
  }  
  if ( currentMillis - Alarm_ElapsedTime >= Alarm_period){
    Alarm_ElapsedTime = currentMillis;
    Alarm_TickFunc();
    //Serial.print("Running Alarm\n");
  }
  if ( currentMillis - SensorLight_ElapsedTime >= SensorLight_period){
    SensorLight_ElapsedTime = currentMillis;
    SensorLight_TickFunc();
    //Serial.print("Running SensorLight\n");
  }
  if ( currentMillis - DetectTemp_ElapsedTime >= DetectTemp_period){
    DetectTemp_ElapsedTime = currentMillis;
    DetectTemp_TickFunc();
    //Serial.print("Running DetectTemp\n");
  }
}

void getDistance(){
  digitalWrite(trigger, LOW);
  //delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  //delayMicroseconds(10);
  travelTime = pulseIn(echo, HIGH);
  distance = (travelTime * .034)/2;
}

void getTemperature(){
  // Read temperature as Fahrenheit (isFahrenheit = true)
  f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print(f);
  Serial.print(F("°F\n"));
}
