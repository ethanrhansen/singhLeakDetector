#include <ESP8266WiFi.h>

//Connecting to wifi and setting up the device with your IFTTT account
const char* ssid     = "CSMwireless"; //The ssid name for the wifi network for the device to connect to
const char* password = ""; //Password to the wifi network. 

const char* host = "wifitest.adafruit.com";


//Ohmmeter vars
int analogPin= A0; //The analog pin that we read the voltage from the "ohmmeter" circuit is measured.
int Vin= 3.3; //input voltage to the circuit, from the voltage pin on the ESP8266
float Rk= 10000;//known resistor value (in ohms)
float Rthresh = 70000;//threshold resistance, below which a notification should be sent

float Vread= 0; //placeholder variable used in the resistance calculation
float Vout= 0; //initialize variable for the measured voltage on the analog pin 
float Rm= 0; //initialize measured resitance variable
float avgCount= 0; //initialize loop counting varaible for taking the average resistance
float Rsum= 0; //intialize summing varaible for average
float Ravg= 0; //intialize variable for the average resistance over 10 measurements


//links for connecting to IFTTT
//Unique Webhooks Key
String webKey = "REPLACE_WITH_WEBHOOKS_KEY"; 

//Trigger names
String logTrig = "data_log"; //Name of your applet for logging data to a google sheets file
String notifTrig = "send_notif"; //Name of your applet for sending a phone notification
String mailTrig = "send_mail"; //Name of your applet for sending an email

// unique IFTTT URL resource, the links to the IFTTT events that you want to trigger. Nothing to updated
const char* logDat = String("/trigger/" + logTrig + "/with/key/" + webKey).c_str();
const char* notifSend = String("/trigger/" + notifTrig + "/with/key/" + webKey).c_str();
const char* mailSend = String("/trigger/" + mailTrig + "/with/key/" + webKey).c_str();

// Maker Webhooks IFTTT: The link for trigger the online event
const char* server = "maker.ifttt.com";

//notification variables
bool notified= false;//checks if already sent notification
float count= 0;//counter for how many iterations the counter has gone through without sending a notification


void setup() {
  Serial.begin(115200);
  delay(100);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


}

int value = 0;

void loop() {
  delay(500);
  ++value;

//Ohmmeter Function

  vRead= analogRead(analogPin); //read the analog pin. the readout is 1024*voltage (because the max voltage is 1V. If max was 3.3 V, then it would be voltage*1024/3.3
  if(vRead) 
  {
    Vout= (vRead)/1024.0; //the readout from the analog pin is 
    Rm = Rk * ((Vin/Vout) -1); //solve for the resistance from the voltage reading and the known resistor value
    delay(500);
  }
  //Take the average of the resitances over 10 measurements
  if(avgCount < 10){
    Rsum += Rm;
    ++avgCount;
  }
  else {
    Ravg = Rsum/10;
    avgCount = 0;
    Rsum = 0;
    //send notification if resistance is below the threshold
    if(Ravg < Rthresh){
      if(notified == false){
        makeIFTTTRequest(notifSend); //this triggers the app notification event
        makeIFTTTRequest(mailSend); //this triggers the email event
        Serial.print("Leak Detected.");
        notified = true;
      }
      ++count;
      if(count==60){//send a leak notification every hour or so
        count = 0;
        notified = false;
      }
      Serial.print("Average R: ");
      Serial.println(Ravg);
      makeIFTTTRequest(logDat);//send resistance average data to google doc
    }
    else{
      notified = false;
      count = 0;
      Serial.print("Average R: ");
      Serial.println(Ravg);
      makeIFTTTRequest(logDat);//send resistance average data to google doc
    }
  }

  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/testwifi/index.html";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(500);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println(WiFi.macAddress()); //this prints the device MAC address in the event that you must register it with a campus network
  Serial.println();
  Serial.println("closing connection");
}

// Make an HTTP request to the IFTTT web service
void makeIFTTTRequest(const char* key) {
  Serial.print("Connecting to "); 
  Serial.print(server);
  
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Failed to connect...");
  }
  
  Serial.print("Request resource: "); 
  Serial.println(key);

  // Values to post to sheets
  String jsonObject = String("{\"value1\":\"") + Ravg + "\"}";
                      
  client.println(String("POST ") + key + " HTTP/1.1");
  client.println(String("Host: ") + server); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
        
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop(); 
}
