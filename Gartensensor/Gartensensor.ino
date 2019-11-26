#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//Festlegen aller Ein-/Ausgänge
//#define soilPin 14
//#define rainPin 5

#define sensorPin D1
#define dhtInputPin D6
#define rainInputPin D7
#define soilInputPin A0

//Passwörter und Daten für MQTT
#define ssid ""
#define password ""
#define mqtt_server "192.168.178.23"

//Definition der Variablen
int counter = 0;
float humidity, temperature, soildeHum, rain, SoilHum;
DHTesp dht;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Trying to connect to this WiFi-network: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("Finished!");
  Serial.println("This is my IP-adress: Baby, call me if you can! ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mr. Future send my a message in this topic: ");
  Serial.print(topic);
  Serial.print("Here is the message");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Faded! I'm looking for my broker. Do you know where he is? Please call me! ");
    if (client.connect("ESP-Garten")) {
      Serial.println("I found him!");
      client.publish("Garten/Sensor/status", "");
      client.publish("Garten/Sensor/humidity", "");
      client.publish("Garten/Sensor/rain", "");
      client.publish("Garten/Sensor/soildry", "");
      client.publish("Garten/Sensor/soilhum", "");
      client.publish("Garten/Sensor/temperature", "");
    } else {
      Serial.print("I couldn't find him! :( I think it's error ");
      Serial.print(client.state());
      Serial.println(" I need a break! Let my dry my eyes.");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  dht.setup(dhtInputPin, DHTesp::DHT11);
  pinMode(sensorPin, OUTPUT);
  pinMode(soilInputPin, INPUT);
  pinMode(rainInputPin, INPUT);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void loop() {

  digitalWrite(sensorPin, HIGH);
  //delay(dht.getMinimumSamplingPeriod() + 2000);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //Werte holen
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
  for (int i = 0; i < 600; i++) {
    if (analogRead(soilInputPin) >= ! soildeHum + 50) soildeHum += analogRead(soilInputPin);
    if (analogRead(soilInputPin) <= ! soildeHum - 50) soildeHum += analogRead(soilInputPin);
    delay(20);
  }
  soildeHum /= 600;

  for (int i = 0; i < 600; i++) {
    rain += digitalRead(rainInputPin);
    delay(20);
  }
  rain /= 600;

  //Werte verarbeiten
  calculate();
  if (dht.getStatusString() != "TIMEOUT") {
    Serial.print("Status: ");
    Serial.print(dht.getStatusString());
    client.publish("Garten/Sensor/status", dht.getStatusString());

    Serial.println("\t");
    Serial.print("Luftfeuchtigkeit: ");
    Serial.print(humidity, 1);
    snprintf (msg, 50, "test", humidity);
    client.publish("Garten/Sensor/humidity", msg);

    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Temperatur: ");
    Serial.print(temperature, 1);
    snprintf (msg, 50, "test", temperature);
    client.publish("Garten/Sensor/temperature", msg);

    Serial.print("°C");
    Serial.println("\t\t");
    Serial.print("Bodentrockenheit: ");
    Serial.print(soildeHum, 1);
    snprintf (msg, 50, "test", soildeHum);
    client.publish("Garten/Sensor/soildry", msg);

    Serial.println("\t\t");
    Serial.print("Bodenfeuchtigkeit: ");
    Serial.print(SoilHum, 1);
    snprintf (msg, 50, "test", SoilHum);
    client.publish("Garten/Sensor/soilhum", msg);

    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Regenstärke: ");
    Serial.print(rain, 1);
    snprintf (msg, 50, "test", rain);
    client.publish("Garten/Sensor/rain", msg);

    Serial.print("%");
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
  }
  //output();
}
void calculate() {      //Werte der Sensoren durch Rechnung in Prozent

  //Berechnung der Bodenfeuchte in Prozent
  soildeHum /= 1024;
  soildeHum *= 100;
  if (soildeHum <= 0) soildeHum = 0;
  if (soildeHum >= 100) soildeHum = 100;
  //Berechnung der Bodentrockenheit in Prozent
  SoilHum = 100 - soildeHum;
  //Berechnung der Regenstärke in Prozent
  rain *= 100;
  rain = 100 - rain;
  if (rain <= 0) rain = 0;
  if (rain >= 100) rain = 100;
}
void output() {         //Ausgabe der Ergebnisse
  if (dht.getStatusString() != "TIMEOUT") {
    Serial.print("Status: ");
    Serial.print(dht.getStatusString());
    client.publish("Garten/Sensor/status", dht.getStatusString());

    Serial.println("\t");
    Serial.print("Luftfeuchtigkeit: ");
    Serial.print(humidity, 1);
    snprintf (msg, 50, "test", humidity);
    client.publish("Garten/Sensor/humidity", msg);

    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Temperatur: ");
    Serial.print(temperature, 1);
    snprintf (msg, 50, "test", temperature);
    client.publish("Garten/Sensor/temperature", msg);

    Serial.print("°C");
    Serial.println("\t\t");
    Serial.print("Bodentrockenheit: ");
    Serial.print(soildeHum, 1);
    snprintf (msg, 50, "test", soildeHum);
    client.publish("Garten/Sensor/soildry", msg);

    Serial.println("\t\t");
    Serial.print("Bodenfeuchtigkeit: ");
    Serial.print(SoilHum, 1);
    snprintf (msg, 50, "test", SoilHum);
    client.publish("Garten/Sensor/soilhum", msg);

    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Regenstärke: ");
    Serial.print(rain, 1);
    snprintf (msg, 50, "test", rain);
    client.publish("Garten/Sensor/rain", msg);

    Serial.print("%");
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
  }
}
