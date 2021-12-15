// libraries
#include <DHT.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <Servo.h>
#include <ezTime.h>
#include "arduino_secrets.h"
#include <Adafruit_NeoPixel.h>

//Id & password setting
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqttuser = SECRET_MQTTUSER;
const char* mqttpass = SECRET_MQTTPASS;

// MQTT topic
const char topic[]  = "student/CASA0016/ucrhhew/smartbin/temperature";
const char topic1[]  = "student/CASA0016/ucrhhew/smartbin/humidity";
const char topic2[]  = "student/CASA0016/ucrhhew/smartbin/level";//-garbage level
const char topic3[]  = "student/CASA0016/ucrhhew/smartbin/timeopen";//-count open times
const char topic4[]  = "student/CASA0016/ucrhhew/smartbin/distanceOut";

//MQTT Server
const char* mqtt_server = "mqtt.cetools.org";
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
 
//DHT setting
#define debugSerial Serial
#define DHTPIN 8  
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Servo Setting
Servo myservo;  
int pos = 180;

//defines pins numbers      
const int trigPin1 = 7;        // Ultrasonic sensor 
const int echoPin1 = 4;        // Ultrasonic sensor
const int trigPin2 =13;        // Ultrasonic sensor 
const int echoPin2 = 12;

// defines variables 
long durationIn;
long durationOut;
int distanceIn;
int distanceOut;
int level;
float tempLevel = 0;
float totalLevel= 30.00;
int timeopen = 0;
int sen = 0;
int j=0;
boolean push = 1;

//led
#define redLed 0 //lid-state open
#define greenLed 1 // lid-state-close

//led-strip
#define PIN 2
#define NUMPIXELS 8
#define DELAYVAL 500
Adafruit_NeoPixel pixels(NUMPIXELS, PIN);


void setup(){

  debugSerial.begin(9600);
  delay(100);
  //start Wifi
  startWifi();
  // start MQTT server
  mqttClient.connect(mqtt_server, 1884);
  // Wait a maximum of 10s for Serial Monitor
  //while (!debugSerial && millis() < 10000);
  myservo.attach(10);
  myservo.write(pos);
  pinMode(trigPin1, OUTPUT);     // Sets the trigPin as an Output
  pinMode(echoPin1, INPUT); // Sets the echoPin as an Input
  pinMode(trigPin2, OUTPUT);     // Sets the trigPin as an Output
  pinMode(echoPin2, INPUT); 
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  
  //Initialise the DHT sensor
  dht.begin();
  connectMQTT();
  pixels.begin();
  pixels.clear();
  
  //turn on greenLed-bin start working!
  Serial.println("SMART DUSTBIN IS Ready TO WORK");
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, HIGH);
  
}



void loop(){
// updata every second-otherwise the (readdistance&open-close lid) process will delay
 if (secondChanged()) {
  readDistance();
  sendMQTT();
  //Serial.println(GB.dateTime("H:i:s"));
  //delay(5000);
 }
 
}

void readDistance(){
 
   digitalWrite(trigPin2, LOW); // Clears the trigPin
   digitalWrite(trigPin2, HIGH); // Sets the trigPin on HIGH state for 10 micro seconds
   delay(10);
   digitalWrite(trigPin2, LOW);
   durationOut = pulseIn(echoPin2, HIGH); // Reads the echoPin
   // Calculate the distance (Garbage in the bin)
   distanceOut = durationOut*0.034/2; 
   
   
   debugSerial.print("DistanceOut: ");
   debugSerial.println(distanceOut);
   
   
  if (distanceOut < 10){
    
    //turn on red light-lid open!
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, HIGH);
    
//referring start-but made some changes
//https://github.com/honhon01/Smart-Waste-Bin/tree/master/Arduino_SmartWasteBin
    if(push == 0) // If the bin previously closed
      {
      push = 1; // Set that bin is open
      myservo.write(pos-180);
      delay(10000);
      timeopen++;  // Count Time open
      Serial.print("Open the lip---------- " + String(timeopen)+ "\n");
      
      }
    } 
  else 
  {  
      push = 0; 
      myservo.write(pos);
      digitalWrite(redLed, LOW);
      digitalWrite(greenLed, HIGH);
      
      // when the lid is close, start measuring
      Serial.print("Lid state:close-start to measure------- \n");
      
      //read temperature and humidity
      uint16_t temperature = dht.readTemperature(false); //false=Celsius, true=Farenheit
      uint16_t humidity = dht.readHumidity(false); //Read humidity values
      
      // Calculate the distance (Garbage in the bin)
      digitalWrite(trigPin1, LOW); // Clears the trigPin
      digitalWrite(trigPin1, HIGH); // Sets the trigPin on HIGH state for 10 micro seconds
      delay(10);
      digitalWrite(trigPin1, LOW);
      durationIn = pulseIn(echoPin1, HIGH); // Reads the echoPin
      distanceIn = durationIn*0.034/2; 
      //debugSerial.print("DistanceIn: ");
      //debugSerial.println(distanceIn);

      if(distanceIn > 30){
        distanceIn = 30;}
      else if(distanceIn < 0){
        distanceIn = 0;}
      //Calculate level of garbage in %
      tempLevel = distanceIn/totalLevel * 100;
      level = 100 - tempLevel;
      
      /*
      debugSerial.print("Temperature: ");
      debugSerial.println(temperature);
      debugSerial.print("Humidity: ");
      debugSerial.println(humidity);
      debugSerial.print("level: ");
      debugSerial.println(level);
      debugSerial.print("timeopen: ");
      debugSerial.println(timeopen);
      */
 //******************************referring end

      
      stripLed();
      
      }
  }
  //delay(10000);


void startWifi(){
  // connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // check to see if connected and wait until you are
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT(){
 mqttClient.setUsernamePassword(mqttuser,mqttpass);
 Serial.println("You're connected to the network");   
 Serial.println();  
 Serial.print("Attempting to connect to the MQTT broker: ");     
 Serial.println(mqtt_server); 
 if (!mqttClient.connect(mqtt_server, 1884)) {        
    Serial.print("MQTT connection failed! Error code = ");       
    Serial.println(mqttClient.connectError());  
      while (1);     
    } 
   Serial.println("You're connected to the MQTT broker!");     
   Serial.println();
  
  }
  
void sendMQTT() {
    mqttClient.available();
    mqttClient.poll();
    uint16_t  Temperature = dht.readTemperature();
    Serial.print("Publish message for bin-temperature: ");//change
    Serial.println(topic);
    Serial.println(Temperature);

    uint16_t Humidity = dht.readHumidity();
    Serial.print("Publish message for bin-humidity: ");
    Serial.println(topic1);
    Serial.println(Humidity);

    Serial.print("Publish message for garbage level: ");
    Serial.println(topic2);
    Serial.println(level);

    Serial.print("Publish message for lid-timeopen: ");
    Serial.println(topic3);
    Serial.println(timeopen);
    
/*
    Serial.print("Publish message for In: ");
    Serial.println(topic4);
    Serial.println(distanceIn);
*/
    Serial.print("Publish message for object2bin-distance: ");
    Serial.println(topic4);
    Serial.println(distanceOut);


    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(Temperature);
    mqttClient.endMessage();

    mqttClient.beginMessage(topic1);
    mqttClient.print(Humidity);
    mqttClient.endMessage();

    mqttClient.beginMessage(topic2);
    mqttClient.print(level);
    mqttClient.endMessage();

    mqttClient.beginMessage(topic3);
    mqttClient.print(timeopen);
    mqttClient.endMessage();
    
    mqttClient.beginMessage(topic4);
    mqttClient.print(distanceOut);
    mqttClient.endMessage();
}
// sometimes max level is 86 not 100
void stripLed(){
  pixels.clear();
  Serial.println(level); 
  if ((0<=level) && (level<=10)){
    pixels.clear();
    }
  
 //Serial.println(level);//避免误差0-90 max-86
  else if ((10<level) && (level<=20)){
    for(int i=0; i<1; i++) {
      pixels.setPixelColor(i, 255,105,180); 
    }
  }
  else if ((20<level) && (level<=30)){
    for(int i=0; i<2; i++) {
      pixels.setPixelColor(i, 255,105,180); 
    }
  }
  else if ((30<level) && (level<=40)){
    for(int i=0; i<3; i++) {
      pixels.setPixelColor(i, 255,105,180); 
    }
  }
  else if ((40<level) && (level<=50)){
    
    for(int i=0; i<4; i++) {
      pixels.setPixelColor(i, 255,105,180); 
    }
  }
  
  else if ((50<level) && (level<=60)){
    for(int i=0; i<5; i++) {
      pixels.setPixelColor(i, 255,105,180); 
    }
  }
  else if ((60<level) && (level<=70)){
    for(int i=0; i<6; i++) {
      pixels.setPixelColor(i, 255,105,180);
    }
  }
  else if ((70<level) &&(level<=80)){
    for(int i=0; i<7; i++) {
      pixels.setPixelColor(i, 153,0,153);
    }
  }
  else if ((80<level) &&(level<=100)){ 
    for(int i=0; i<8; i++) {
      pixels.setPixelColor(i, 150, 0, 0);
    }
   Serial.println("Bin is full!");
   pixels.clear(); 
  }
  
  //else {
  //pixels.clear();
    //}
  pixels.show();
  }
 
