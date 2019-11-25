#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include <MQTTClient.h>

//Festlegen aller Ein-/Ausgänge
#define dhtInputPin 12
#define soilPin 14
#define rainPin 5
#define soilRainInputPin A0

//Passwörter und Daten für MQTT
#define ssid "
#define password ""
#define mqtt_server "192.168.178.23"
//Definition der Variablen
int counter = 0;
float humidity, temperature, soildeHum, rain, Soil, Rain, SoilHum;
DHTesp dht;
WiFiClient espClient;
long lastMsg = 0;
char msg[50];
int value = 0;
void setup() {
  Serial.begin(115200);
  Serial.println();
  dht.setup(dhtInputPin, DHTesp::DHT11);
  pinMode(soilPin, OUTPUT);
  pinMode(rainPin, OUTPUT);
  Serial.println("GartenSensor - by Maximilian Inckmann");
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Versuche mich mit WLAN zu verbinden:  Netzwerkname: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("Ich habe mich erfolgreich verbunden!");
  Serial.println("Meine lokale IP-Adresse lautet: ");
  Serial.println(WiFi.localIP());
  Serial.print("Verbinde mit MQTT Server ....");
  client.begin("192.168.178.23", WiFiclient);
  conn();
}
void loop() {
  //Alles klarmachen -> Setup für jeden Durchlauf
  toggle();
  delay(dht.getMinimumSamplingPeriod() + 2000);
  client.loop();
  if(!client.connected()) {
    connect();
  }
  //Werte holen
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
  if (counter == 0) {
    for (int i = 0; i < 600; i++) {
      if (analogRead(A0) >= ! Soil + 50) soildeHum += analogRead(A0);
      if (analogRead(A0) <= ! Soil - 50) soildeHum += analogRead(A0);
      delay(200);
    }
    soildeHum /= 600;
  }
  else if (counter == 1) {
    for (int i = 0; i < 600; i++) {
      rain += analogRead(A0);
      delay(200);
    }
    rain /= 600;
  }
  //Werte verarbeiten
  calculate();
  output();
}
void conn() {
 while (!client.connect("nodemcu", "try", "try")) {
   Serial.print(".");
 }
 
 Serial.println("\nconnected!");
 client.subscribe("nodemcu");
}
void calculate() {      //Werte der Sensoren durch Rechnung in Prozent
  int differenz = 1024 - (soildeHum + rain);
  if (differenz > 0) {
    if (soildeHum > 512) {
      Soil = soildeHum / (1 + ((soildeHum - 512) / soildeHum));
    }
    if (rain > 512) {
      Rain = rain / (1 + ((rain - 512) / rain));
    }
    else if (soildeHum < 512) {
      Soil = soildeHum * (1 + ((soildeHum - 512) / soildeHum));
    }
    else if (rain < 512) {
      Rain = rain * (1 + ((rain - 512) / rain));
    }
  }
  //Berechnung der Bodenfeuchte in Prozent
  Soil /= 512;
  Soil *= 100;
  //Berechnung der Bodentrockenheit in Prozent
  SoilHum = 100 - Soil;
  //Berechnung der Regenstärke in Prozent
  Rain /= 512;
  Rain *= 100;
}
void toggle() {         //Schalten der Sensoren
  if (counter == 1) {
    counter = 0;
    digitalWrite(rainPin, LOW);
    delay(1000);
    digitalWrite(soilPin, HIGH);
  }
  else if (counter == 0) {
    counter = 1;
    digitalWrite(soilPin, LOW);
    delay(1000);
    digitalWrite(rainPin, HIGH);
  }
}
void output() {         //Ausgabe der Ergebnisse
  if (dht.getStatusString() != "TIMEOUT") {
    Serial.print("Status: ");
    Serial.print(dht.getStatusString());
    client.publish("GartenSensor/status", (String)dht.getStatusString());
    
    Serial.println("\t");
    Serial.print("Luftfeuchtigkeit: ");
    Serial.print(humidity, 1);
 
    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Temperatur: ");
    Serial.print(temperature, 1);
    
    Serial.print("°C");
    Serial.println("\t\t");
    Serial.print("Bodentrockenheit: ");
    Serial.print(SoilHum, 1);
   
    Serial.println("\t\t");
    Serial.print("Bodenfeuchtigkeit: ");
    Serial.print(Soil, 1);
    
    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Regenstärke: ");
    Serial.print(Rain, 1);
    
    Serial.print("%");
    Serial.println();
    Serial.println();
    Serial.println(); 
    Serial.println();
  }
}
