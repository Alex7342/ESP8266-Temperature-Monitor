#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Network credentials
const char* ssid = "Alex";
const char* password = "danimocanuboss";

// Server port
const int PORT = 80;

// Web server
ESP8266WebServer server(PORT);

// The time between temperature readings
const int timeInterval = 5000;

// The time of the last temperature reading
int lastReadingTime = 0;

// The value of the last temperature reading in celcius
float lastCelciusTemperatureReading = -127.0;

// GPIO the sensor is connected to
const int oneWireBus = 4;

// OneWire instance to communicate with devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP8266 Temp</title>
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background-color: #f4f6f8;
      color: #333;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      padding: 20px;
      box-sizing: border-box;
    }
    .card {
      background: white;
      padding: 30px 40px;
      border-radius: 12px;
      box-shadow: 0 8px 16px rgba(0,0,0,0.1);
      text-align: center;
      width: 100%;
      max-width: 400px;
    }
    h2 {
      margin-bottom: 20px;
      color: #0077cc;
      font-size: 1.8em;
    }
    #temp {
      font-size: 2.5em;
      font-weight: bold;
      color: #222;
    }
    .footer {
      margin-top: 15px;
      font-size: 0.9em;
      color: #888;
    }

    /* Mobile tweaks */
    @media (max-width: 600px) {
      .card {
        padding: 20px;
      }
      h2 {
        font-size: 1.5em;
      }
      #temp {
        font-size: 2em;
      }
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>ESP8266 Temperature</h2>
    <p id="temp">--</p>
    <div class="footer">Updated every 2 seconds</div>
  </div>

  <script>
    setInterval(() => {
      fetch('/temperature')
        .then(res => res.json())
        .then((res) => {
          document.getElementById('temp').innerHTML = res.temp + ' °C';
        });
    }, 2000);
  </script>
</body>
</html>
)rawliteral";


  server.send(200, "text/html", html);
}

void handleTemperature() {
  String json = "{\"temp\":" + String(lastCelciusTemperatureReading) + "}";

  server.send(200, "application/json", json);
}

void setup() {
  // Start the Serial Monitor
  Serial.begin(115200);
  Serial.println();

  // Start the DS18B20 sensor
  Serial.println("Starting sensors...");
  sensors.begin();

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("Connected!");
  Serial.println("IP address: " + WiFi.localIP().toString());
  Serial.println("Port: " + String(PORT));

  // Server setup
  server.on("/", handleRoot);
  server.on("/temperature", handleTemperature);
  server.begin();
}

void loop() {
  // Update the temperature if needed
  unsigned long currentTime = millis();
  if (currentTime - lastReadingTime >= timeInterval) {
    sensors.requestTemperatures();
    lastCelciusTemperatureReading = sensors.getTempCByIndex(0);
    lastReadingTime = currentTime;
  }

  server.handleClient();
}