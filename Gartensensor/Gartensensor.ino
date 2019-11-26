#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//Festlegen aller Ein-/Ausgänge
#define dhtInputPin 12
#define soilPin 14
#define rainPin 5
#define soilRainInputPin A0

//Passwörter und Daten für MQTT
#define ssid ""
#define password ""
#define mqtt_server "192.168.178.23"

//Definition der Variablen
int counter = 0;
float humidity, temperature, soildeHum, rain, Soil, Rain, SoilHum;
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
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Faded! I'm looking for my broker. Do you know where he is? Please call me! ");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Now we're in love! I found him.");
      // Once connected, publish an announcement...
      client.publish("status", "Hello, can you hear me?!");
      // ... and resubscribe
      client.subscribe("test");
    } else {
      Serial.print("I couldn't find him! :( I think it's error ");
      Serial.print(client.state());
      Serial.println("I need a break! Let my dry my eyes.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  dht.setup(dhtInputPin, DHTesp::DHT11);
  pinMode(soilPin, OUTPUT);
  pinMode(rainPin, OUTPUT);
  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

//void setup() {
//  Serial.begin(115200);
//  Serial.println();
//  dht.setup(dhtInputPin, DHTesp::DHT11);
//  pinMode(soilPin, OUTPUT);
//  pinMode(rainPin, OUTPUT);
//  Serial.println("GartenSensor - by Maximilian Inckmann");
//  delay(10);
//  // We start by connecting to a WiFi network
//  Serial.println();
//  Serial.print("Versuche mich mit WLAN zu verbinden:  Netzwerkname: ");
//  Serial.println(ssid);
//  WiFi.begin(ssid, password);
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//  randomSeed(micros());
//  Serial.println("");
//  Serial.println("Ich habe mich erfolgreich verbunden!");
//  Serial.println("Meine lokale IP-Adresse lautet: ");
//  Serial.println(WiFi.localIP());
//  Serial.print("Verbinde mit MQTT Server ....");
//  //client.begin("192.168.178.23", WiFiclient);
//  conn();
//}
void loop() {
  //Alles klarmachen -> Setup für jeden Durchlauf
  toggle();
  delay(dht.getMinimumSamplingPeriod() + 2000);
  client.loop();
  if (!client.connected()) {
    conn();
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
    client.publish("GartenSensor/status", dht.getStatusString());

    Serial.println("\t");
    Serial.print("Luftfeuchtigkeit: ");
    Serial.print(humidity, 1);
    //client.publish("GartenSensor/humidity", char*(humidity));

    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Temperatur: ");
    Serial.print(temperature, 1);
    client.publish("GartenSensor/temperature", temperature.toString());

    Serial.print("°C");
    Serial.println("\t\t");
    Serial.print("Bodentrockenheit: ");
    Serial.print(SoilHum, 1);
    client.publish("GartenSensor/soildry", (String)SoilHum);

    Serial.println("\t\t");
    Serial.print("Bodenfeuchtigkeit: ");
    Serial.print(Soil, 1);
    client.publish("GartenSensor/soilhum", (String)Soil);

    Serial.print("%");
    Serial.println("\t\t");
    Serial.print("Regenstärke: ");
    Serial.print(Rain, 1);
    client.publish("GartenSensor/rain", (String)humidity);

    Serial.print("%");
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
  }
}
