#include <Arduino.h>
#include <LiquidCrystal.h>

#include <WiFi.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "settings.h"

// Create An LCD Object. Signals: [ RS, EN, D4, D5, D6, D7 ]
LiquidCrystal My_LCD(13, 12, 14, 27, 26, 25);

const char* ssid = SSID;           
const char* password = SSID_PASSWORD; 
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);


void setup(void) {
  delay(2000);
  Serial.begin(115200);
  Serial.println();

  My_LCD.begin(16, 2);
  My_LCD.clear();


  /*Serial.println(TFT_MOSI); // 23
  Serial.println(TFT_SCLK); // 18
  Serial.println(TFT_DC); //   2
  Serial.println(TFT_RST); //  4
  Serial.println(TFT_CS); //   15*/

  //tft.init();

  //setDisplayOn(true);
  //tft.fillScreen(TFT_BLACK);

  // Set "cursor" at top left corner of display (0,0) and select font 4
  //tft.setCursor(0, 0, 4);
  // Set the font colour to be white with a black background
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // We can now plot text on screen using the "print" class
  //tft.println("Initialised\n");


  WiFi.begin(ssid, password);

  //Serial.print("Connecting");
  My_LCD.print("Connecting");
  Serial.println("Connecting\n");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    My_LCD.print(".");
    Serial.print(".");
  }
  //tft.println("");

  //tft.print("Connected, IP address: ");
  //tft.println(WiFi.localIP());
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  // Check server connection
  if (client.validateConnection()) {
    //tft.print("Connected to InfluxDB: ");
    //tft.println(client.getServerUrl());
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    //tft.print("InfluxDB connection failed: ");
    //tft.println(client.getLastErrorMessage());
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

 

}

void loop() {
  My_LCD.clear();

  String query = "from(bucket: \"power\") ";
  query += "|> range(start: -15m) ";
  query += "|> filter(fn: (r) => r[\"_measurement\"] == \"battery\" and ";
  query += "r[\"_field\"] == \"SOC\" or r[\"_field\"] == \"Pbat\" or r[\"_field\"] == \"PmeterL1\") ";
  //query += "|> filter(fn: (r) => r[\"_field\"] == \"SOC\") ";
  query += "|> last()";
  // aggregateWindow(every: 15m, fn: mean, createEmpty: false) |> yield(name: \"mean\")";

  Serial.println("==== List results ====");

    // Print composed query
  Serial.print("Querying with: ");
  Serial.println(query);

  // Send query to the server and get result
  FluxQueryResult result = client.query(query);
  double SOC = 0;
  double Pbat = 0;
  double PmeterL1 = 0;

  while (result.next()) {
    String field = result.getValueByName("_field").getString();
    Serial.print(field);
    Serial.print(": ");


    if(field == "SOC") {
      SOC = result.getValueByName("_value").getDouble();
      Serial.println(SOC);

    }
    if(field == "Pbat") {
      Pbat = result.getValueByName("_value").getDouble();
      Serial.println(Pbat);
    }


    if(field == "PmeterL1") {
      PmeterL1 = result.getValueByName("_value").getDouble(); 
      Serial.println(PmeterL1);
      //tft.print("Grid: ");
      //tft.println(value,0);
    }
  }
  My_LCD.setCursor(0,0);
  if (SOC <= 12.8) {
    My_LCD.print("Battery Empty");
  }
  else if (SOC == 100.0) {
    My_LCD.print("Battery Full");
  }
  else {
    My_LCD.print("Batt Lvl: ");
    My_LCD.print(SOC,1);
    My_LCD.print("%");
  }
  My_LCD.setCursor(0,1);
  if(Pbat != 0) {
    if(Pbat > 0) {
      My_LCD.print("Discharge: ");
    }
    else {
      Pbat = abs(Pbat);
      My_LCD.print("Charging: ");
    }
    //My_LCD.print("Batt Use: ");
    My_LCD.print(Pbat,0);
  }
  else {
    if (PmeterL1 > 0) {
      My_LCD.print("Grid Use: ");
      My_LCD.print(PmeterL1,0);
    }
    else {
      PmeterL1 = abs(PmeterL1);
      My_LCD.print("Feed-in: ");
      My_LCD.print(PmeterL1,0);
    }
    
  }

  // Check if there was an error
  if(result.getError().length() > 0) {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }


  // Close the result
  result.close();

  delay(10000);
}
