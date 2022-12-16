#include "Arduino.h"
#include "config.h"
#include <WiFi.h>
#include <InfluxDbClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>

#ifdef SENSOR_USE_BME280
  #include <Adafruit_BME280.h>
  Adafruit_BME280 sensor;
#endif
#ifdef SENSOR_USE_BMP280
  #include <Adafruit_BMP280.h>
  Adafruit_BMP280 sensor;
#endif


InfluxDBClient influx_client(INFLUX_URL, INFLUX_ORG, INFLUX_BUCKET, INFLUX_TOKEN);
Point influx_point("climate");

unsigned long previous_millis = 0;
unsigned long current_millis = 0;
unsigned long interval = 30000;



void wifi_init() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void wifi_check(){
  if ((WiFi.status() != WL_CONNECTED) && (current_millis - previous_millis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previous_millis = current_millis;
  }
}


void setup() {
  Serial.begin(SERIAL_BAUD);
  
  wifi_init();
  bool sensor_status;

  sensor_status = sensor.begin(0x76);
  delay(100);
  if (!sensor_status) {
    Serial.println("BME/BMP connection error");
    while (1) delay(10);
  }

  influx_point.addTag("device", DEVICE_NAME);
  timeSync(TZ_INFO, "pool.ntp.org", "ch.pool.ntp.org");

  if (influx_client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(influx_client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(influx_client.getLastErrorMessage());
  }


  // debug read
  Serial.print("pressure=");
  Serial.println( sensor.readPressure() );
  Serial.print("temperature=");
  Serial.println( sensor.readTemperature() );

  #ifdef SENSOR_USE_BME280
    Serial.print("humidiy");
    Serial.println(sensor.readHumidity());
  #endif

}


void update_datapoint(){

  influx_point.clearFields();
  influx_point.addField("pressure", sensor.readPressure());
  influx_point.addField("temperature", sensor.readTemperature());

  #ifdef SENSOR_USE_BME280
    influx_point.addField("humidity", sensor.readHumidity());
  #endif

  Serial.print("Writing: ");
  Serial.println(influx_point.toLineProtocol());

  if (!influx_client.writePoint(influx_point)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(influx_client.getLastErrorMessage());
  }

}




void loop() {
  current_millis = millis();
  wifi_check();
  update_datapoint();
  delay(UPDATE_INTERVAL*1000);
}
