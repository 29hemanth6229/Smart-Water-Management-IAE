#include <ESP8266WiFi.h>         // Library required to use NodeMcu
#include <Firebase_ESP_Client.h> // Library to access the Firebase from ESP8266(NodeMCU)
#include "addons/TokenHelper.h"
#include <Wire.h>

#define FIREBASE_HOST "iot-project-9ddb6-default-rtdb.firebaseio.com"     // Host ID for FireBase
#define FIREBASE_AUTH "KkBfIXoR8nsbY5cdI42UfFpqBhvrRh2d7s8WGVwo"          // Authentication ID for FireBase
#define WIFI_SSID "Hemanth"       
#define WIFI_PASSWORD "123456789"

#define API_KEY "AIzaSyD0vaglqUcQHwdWvJwSQOhPykWDjdAJlFo"                      // API Key for FireBase
#define DATABASE_URL "https://iot-project-9ddb6-default-rtdb.firebaseio.com/"  // URL for the Database

FirebaseData fbdo;      //Define Firebase Data Object 
FirebaseAuth auth;      //Define Firebase Authentication Object
FirebaseConfig config;  //Define Firebase Configuration Object
bool signupOK=false;    
 
String apiKey = "9LA7X0K20575KS42";  //Write API key from ThingSpeak
const char *ssid = "Hemanth";        //wifi ssid and key
const char *pass = "123456789";
const char* server = "api.thingspeak.com";
 
#define SENSOR  2  // GPIO-2 or D4 for the input from WaterFlow Sensor
String stringData1;
const int relay = 5;
 
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5; // For YF-S201 calibrationFactor is 4.5
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;
float unitcost = 1; // So, the unit cost is "1", that means Rs.1 for each Litre. 
float bill;
 
void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}
 
WiFiClient client;
 
void setup()
{
  Serial.begin(115200);
  delay(10);
  pinMode(SENSOR, INPUT_PULLUP);         // Connecting NodeMCU to Internet

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                  
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("Connected");
  Serial.print("IP Address: ");   
  Serial.println(WiFi.localIP()); //prints local IP address

  config.api_key = API_KEY;                        // Authentication of Firebase accessing account
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
    }
  else{
    Serial.printf("%s\n",config.signer.signupError.message.c_str());
    }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;
 
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);   // Interrupt funciton used in time calculations

  pinMode(relay, OUTPUT);

}
 
void loop()
{
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) 
  {
    
    pulse1Sec = pulseCount;
    pulseCount = 0;
/* Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use
    that to scale the output. We also apply the calibrationFactor to scale the output based on the number of pulses per second per units of measure (litres/minute in this case)
    coming from the sensor. */
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    flowLitres = (flowRate / 60);
 
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    totalLitres += flowLitres;
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(float(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space
 
    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalLitres);
    Serial.println("L");
    bill = (totalLitres)*(unitcost);
    Serial.print("Bill = ");
    Serial.println(bill);
    
    // Sending the values to ThingSpeak
      if(Firebase.ready() && signupOK){
        Firebase.RTDB.setFloat(&fbdo,"Apartment or Colony/House-2 Volume in Litres",totalLitres);
        Firebase.RTDB.setFloat(&fbdo,"Apartment or Colony/House-2 Amount in Rupees",bill);
        }

      if (Firebase.RTDB.getString(&fbdo,"Apartment or Colony/House-2 Valve:")){
      if (fbdo.dataType() == "string"){
        stringData1 = fbdo.stringData();
        //Serial.println(stringData1);
        }
      }

    char arr[6];
    for(int i=0;i<6;i++){
      arr[i]=stringData1[i];
      }
    //Serial.println(arr);
     
    if(arr[3] == 'N'){
      Serial.println("Valve is ON");
      digitalWrite(relay, HIGH);
      }
    if(arr[3] =='F'){
      Serial.println("Valve is OFF");
      digitalWrite(relay, LOW);
      }
      
    delay(1000);    
  }

  // Sending the values to ThingSpeak 
  if (client.connect(server, 80)) // "184.106.153.149" or api.thingspeak.com
  {
    String postStr = apiKey;
      postStr += "&field1=";
      postStr += String(float(flowRate));
      postStr += "&field2=";
      postStr += String(totalLitres);
      postStr += "&field3=";
      postStr += String(bill);
      postStr += "\r\n\r\n";
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
   
  }
    client.stop();
}
