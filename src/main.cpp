#include <Arduino.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_DotStar.h>
#include "Adafruit_LTR329_LTR303.h"

#define HALL_SENSOR_PIN 1
#define NUMPIXELS 1
#define DATAPIN    33
#define CLOCKPIN   21

Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
WiFiClient networkClient; 
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LTR329 ltr = Adafruit_LTR329();


// WiFi network name and password:
const char *networkName = "Jenisha";
const char *networkPswd = "Sjokolade21";

// Internet address to send POST data to
const char *hostDomain = "172.20.10.2";
const int hostPort = 3001;

const int LED_PIN = LED_BUILTIN;

void connectToWiFi(const char *ssid, const char *pwd)
{
  Serial.println("Connecting to WiFi network: " + String(ssid));
  WiFi.begin(ssid, pwd); // start connecting to the wifi network

  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink LED while we're connecting:
    digitalWrite( LED_BUILTIN , !digitalRead( LED_BUILTIN ));
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}



void requestURL(const char *host, int port, int X, int Y, int Z)
{
  Serial.println("Connecting to domain: " + String(host) + ":" + port + " with param X: " + X);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port))
  {
    Serial.println("connection failed");
    return;
  }
  Serial.println("Connected!\n");

  // This will send the POST request to the server
  String dataToSend = String("Temperature=" + String(X) + "&Humidity=" + String(Y) + "&Light=" + String(Z));
  Serial.println(dataToSend);

  int dataStringLength = dataToSend.length();
  client.print((String) "POST /products HTTP/1.1\r\n" +
               "Host: " + String(host) + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + dataStringLength + "\r\n" +
               "Connection: close\r\n\r\n" +
               dataToSend);
  // If something goes wrong, we need a timeout
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available())
  {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  // When we are finished, we close the connection
  Serial.println("closing connection");
  client.stop();
}

void setup()
{
  Serial.begin(115200);

  if ( ! ltr.begin() ) {
    Serial.println("Couldn't find LTR sensor!");
    while (1) delay(10);
  }

  // Setup LTR sensor (see advanced demo in library for all options!)
  Serial.println("Found LTR sensor!");
  ltr.setGain(LTR3XX_GAIN_4);
  ltr.setIntegrationTime(LTR3XX_INTEGTIME_50);
  ltr.setMeasurementRate(LTR3XX_MEASRATE_50);

  if (!sht31.begin(0x44))
  {
    Serial.println("Couldn't find SHT31");
    while (1)
      delay(1);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(HALL_SENSOR_PIN, INPUT);

  }

  delay(3000);
  connectToWiFi(networkName, networkPswd);
  delay(1000);
}

void loop()
{

  
  bool sensorReadingDidWork = true;
  float Temperature = sht31.readTemperature();
  float Humidity = sht31.readHumidity();


  if (isnan(Temperature) || isnan(Humidity)) {  // check if 'is not a number'
      sensorReadingDidWork = false;
  }

  uint16_t visible_plus_ir, infrared;
  if (ltr.newDataAvailable())
  {
    bool valid = ltr.readBothChannels(visible_plus_ir, infrared);
    if (valid)
    {
      Serial.print("CH0 Visible + IR: ");
      Serial.print(visible_plus_ir);
      Serial.print("\tCH1 Infrared: ");
      Serial.println(infrared);
    }
  }
  if (sensorReadingDidWork)
  {
    requestURL(hostDomain, hostPort, Temperature, Humidity, visible_plus_ir);
    Serial.println("POST req sent!");
  }
}
