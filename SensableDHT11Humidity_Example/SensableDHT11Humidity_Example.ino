/*
  DHCP-based sensable reporter based on the IP Printer example from arduino
 
 This sketch uses the DHCP extensions to the Ethernet library
 to get an IP address via DHCP and print the address obtained.
 using an Arduino Wiznet Ethernet shield.
 Reads out the data of a DHT11 to publish its data via a post request to sensable.io
 
 Required Libraries
 SPI, Ethernet, dth11, aJSON
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * DHT11 attached to pins A1
 
 tested on a Arduino Uno R3 with a W5100 ethernet shield
 
 created 10 July 2014
 modified 20 July 2014
 by Christian Kaatz
 */

#include <SPI.h>
#include <Ethernet.h>
#include <dht11.h>
#include <aJSON.h>

dht11 DHT11;

#define DHT11PIN A1
#define ACCESSTOKEN      "YOUR ACCESS TOKEN GOES HERE" // replace your sensable.io access token here

// assign a MAC address for the ethernet controller.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// fill in your address here:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip(10,0,1,20);
// initialize the library instance:
EthernetClient client;
char server[] = "sensable.io";    // name address for sensable.io (using DNS)
char sensorId[] = "gemini-humidity";
boolean firstTime = true;

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
const unsigned long postingInterval = 300*1000; //delay between updates to Sensable.io

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);


  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip);
  }
  delay(1000);
  // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
}

void loop() {

  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

  // if you're not connected, and 300 seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    // read the analog sensor:
    int chk = DHT11.read(DHT11PIN);
    if(chk == DHTLIB_OK) {
      Serial.print("Humidity (%): ");
      Serial.println((float)DHT11.humidity, 2);
      sendData((float)DHT11.humidity);
    } 
    else {
      Serial.println("error reading sensor");
    }
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

// this method makes a HTTP connection to the server:
void sendData(float thisData) {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    String json = getJsonString(thisData);
    if(firstTime) {
      // send the HTTP POST request to create the sensable:
      client.println("POST /sensable HTTP/1.1");
      firstTime = false;
    } else {
      // send the HTTP POST request with the sample only:
      client.print("POST /sensed/");
      client.print(sensorId);
      client.println(" HTTP/1.1");
    }
    client.println("Host: sensable.io");
    client.print("Content-Length: ");

    // calculate the length of the sensor reading in bytes:
    // 8 bytes for "sensor1," + number of digits of the data:
    //    int thisLength = 8 + getLength(thisData);
    client.println(json.length());

    // last pieces of the HTTP PUT request:
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.println(json);
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  // note the time that the connection was made or attempted:
  lastConnectionTime = millis();
}

String getJsonString(float val) {
    aJsonObject *root, *sampleObject;
    sampleObject = aJson.createObject();
    root=aJson.createObject();
    if(firstTime) {
      // set your sensor id (have to be unique)
      aJson.addStringToObject(root, "sensorid", sensorId);
      // align your dataset
      aJson.addStringToObject(root, "unit", "%");
      aJson.addStringToObject(root, "sensortype", "humidity");
      // set the location of the sensor
      double locArray[] = {13.639427,52.209564};
      aJson.addItemToObject(root, "location", aJson.createFloatArray(locArray,2));
    }
    // put your access token, retrieved from sensable.io here
    aJson.addStringToObject(root, "accessToken", ACCESSTOKEN);
    // building up our json object to be posted over to sensable.io
    aJson.addNumberToObject(sampleObject, "value", val);
    // this example does not have itc support, so we have to simply push the data marked as now
    // the server will then generate the timestamp
    aJson.addStringToObject(sampleObject,"timestamp", "now");
    aJson.addItemToObject(root, "sample", sampleObject);
    char *json = aJson.print(root);
    String retval(json);
    free(json);
    aJson.deleteItem(root);
    return retval;
}


