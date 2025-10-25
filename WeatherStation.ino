#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

// Network credentials
const char* ssid = "-";
const char* password = "-";

// Server port
const int PORT = 80;

// Database server URL (your Express server)
String POST_URL = "http://192.168.1.137:3000/submit";
String GET_URL = "http://192.168.1.137:3000/history";

// Web server
ESP8266WebServer server(PORT);

//WiFi client
WiFiClient wifiClient;
// Http client
HTTPClient httpClient;

#define DHTPIN 5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// The time between temperature readings
const int timeInterval = 10000;

// The time of the last temperature reading
unsigned long lastReadingTime = 0;

// The value of the last temperature reading in celcius
float lastCelciusTemperatureReading = -127.0;

// The value of the last humidity reading
float lastHumidityReading = 0.0;


void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Weather Station</title>
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
          font-size: 2em;
        }
        .reading {
          margin: 10px 0;
        }
        .label {
          font-size: 1em;
          color: #666;
          margin-bottom: 4px;
        }
        .value {
          font-size: 1.6em;
          font-weight: bold;
          color: #222;
        }
        .footer {
          margin-top: 20px;
          font-size: 0.9em;
          color: #888;
        }
        .button {
          background-color: #0077cc;
          color: white;
          padding: 12px 24px;
          border: none;
          border-radius: 8px;
          font-size: 1.1em;
          cursor: pointer;
          margin-top: 20px;
          margin-right: 10px;
        }
        .button:hover {
          background-color: #006bb0;
        }

        @media (max-width: 600px) {
          .card {
            padding: 20px;
          }
          h2 {
            font-size: 1.6em;
          }
          .value {
            font-size: 1.4em;
          }
        }
      </style>
    </head>
    <body>
      <div class="card">
        <h2>Weather Station</h2>
        <div class="reading">
          <div class="label">Temperature</div>
          <div class="value" id="temp">--</div>
        </div>
        <div class="reading">
          <div class="label">Humidity</div>
          <div class="value" id="humidity">--</div>
        </div>
        <div>
          <button class="button" onclick="location.href='history'" aria-label="View history of weather data">View History</button>
          <button class="button" onclick="location.href='allReadings'" aria-label="View all recorded weather data">View All Readings</button>
        </div>
        <div class="footer">Updated every 10 seconds</div>
      </div>

      <script>
        function updateReadings() {
          fetch('/reading')
            .then(res => res.json())
            .then((res) => {
              document.getElementById('temp').innerHTML = res.temperature + ' °C';
              document.getElementById('humidity').innerHTML = res.humidity + '%';
            });
        }

        updateReadings(); // Initial call
        setInterval(updateReadings, 10000);
      </script>
    </body>
    </html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleReading() {
  String json = "{\"temperature\":"
                   + String(lastCelciusTemperatureReading) + 
                  ",\"humidity\":" + String(lastHumidityReading) +
                "}";

  server.send(200, "application/json", json);
}

void handleHistory() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Temperature & Humidity History</title>
      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
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
          flex-direction: column;
        }
        h1 {
          margin-bottom: 30px;
          color: #0077cc;
          font-size: 2.5em;
        }
        .chart-container {
          display: flex;
          flex-wrap: wrap;
          justify-content: center;
          gap: 20px;
          width: 100%;
          max-width: 1200px;
        }
        .chart-box {
          background-color: white;
          padding: 20px;
          border-radius: 12px;
          box-shadow: 0 8px 16px rgba(0,0,0,0.1);
          text-align: center;
          flex: 1 1 300px;
          max-width: 600px;
        }
        h2 {
          color: #0077cc;
          font-size: 1.8em;
          margin-bottom: 20px;
        }
        canvas {
          width: 100% !important;
          height: auto !important;
        }
        .footer {
          margin-top: 15px;
          font-size: 0.9em;
          color: #888;
        }
        .button {
          background-color: #0077cc;
          color: white;
          padding: 12px 24px;
          border: none;
          border-radius: 8px;
          font-size: 1.2em;
          cursor: pointer;
          margin-top: 20px;
          text-align: center;
        }
        .button:hover {
          background-color: #005fa3;
        }

        @media (max-width: 600px) {
          h1 {
            font-size: 2em;
          }
          h2 {
            font-size: 1.5em;
          }
        }
      </style>
    </head>
    <body>
      <h1>Temperature & Humidity History</h1>

      <div class="chart-container">
        <div class="chart-box">
          <h2>Temperature</h2>
          <canvas id="temperatureChart"></canvas>
        </div>
        <div class="chart-box">
          <h2>Humidity</h2>
          <canvas id="humidityChart"></canvas>
        </div>
      </div>

      <button class="button" onclick="location.href='/'">Return to Home</button>

      <div class="footer">Updated every 10 seconds</div>

      <script>
        let tempChart;
        let humChart;
        const fetchURL = ')rawliteral" + GET_URL + R"rawliteral(';

        function fetchAndUpdateCharts() {
          fetch(fetchURL)
            .then(response => response.json())
            .then(data => {
              let slicedData = data.slice(0, 15);  // Limit to first 15 entries
              slicedData = slicedData.reverse(); // Set the order to "oldest to newest"

              const labels = slicedData.map(entry => new Date(entry.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
              const temps = slicedData.map(entry => entry.temperature);
              const hums = slicedData.map(entry => entry.humidity);

              // Temperature chart
              if (!tempChart) {
                tempChart = new Chart(document.getElementById('temperatureChart'), {
                  type: 'line',
                  data: {
                    labels: labels,
                    datasets: [
                      {
                        label: 'Temperature (°C)',
                        data: temps,
                        borderColor: 'red',
                        fill: false,
                        tension: 0.3
                      }
                    ]
                  },
                  options: {
                    responsive: true,
                    scales: {
                      x: {
                        display: true,
                        title: {
                          display: true,
                          text: 'Time'
                        }
                      },
                      y: {
                        display: true,
                        title: {
                          display: true,
                          text: 'Temperature (°C)'
                        }
                      }
                    }
                  }
                });
              } else {
                tempChart.data.labels = labels;
                tempChart.data.datasets[0].data = temps;
                tempChart.update();
              }

              // Humidity chart
              if (!humChart) {
                humChart = new Chart(document.getElementById('humidityChart'), {
                  type: 'line',
                  data: {
                    labels: labels,
                    datasets: [
                      {
                        label: 'Humidity (%)',
                        data: hums,
                        borderColor: 'blue',
                        fill: false,
                        tension: 0.3
                      }
                    ]
                  },
                  options: {
                    responsive: true,
                    scales: {
                      x: {
                        display: true,
                        title: {
                          display: true,
                          text: 'Time'
                        }
                      },
                      y: {
                        display: true,
                        title: {
                          display: true,
                          text: 'Humidity (%)'
                        }
                      }
                    }
                  }
                });
              } else {
                humChart.data.labels = labels;
                humChart.data.datasets[0].data = hums;
                humChart.update();
              }
            })
            .catch(err => {
              console.error('Error loading history:', err);
              document.body.insertAdjacentHTML('beforeend', '<p style="color:red;">Failed to load data</p>');
            });
        }

        // Initial load
        fetchAndUpdateCharts();

        // Refresh every 10 seconds (10000 milliseconds)
        setInterval(fetchAndUpdateCharts, 10000);
      </script>
    </body>
    </html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleAllReadings() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
      <title>Readings History</title>
      <style>
        body {
          font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
          background-color: #f4f6f8;
          color: #333;
          margin: 0;
          padding: 20px;
          box-sizing: border-box;
        }

        h1 {
          text-align: center;
          color: #0077cc;
          margin-bottom: 30px;
        }

        .subtitle-space {
          height: 10px;
        }

        .button-container {
          display: flex;
          justify-content: flex-start;
          gap: 10px;
          margin: 0 auto 20px auto;
          max-width: 1000px;
        }

        .button {
          background-color: #0077cc;
          color: white;
          padding: 10px 20px;
          border: none;
          border-radius: 8px;
          font-size: 1em;
          cursor: pointer;
        }

        .button:hover {
          background-color: #006bb0;
        }

        .readings-container {
          display: flex;
          flex-direction: column;
          gap: 10px;
          max-width: 1000px;
          margin: 0 auto;
        }

        .reading-card {
          background: white;
          padding: 15px 20px;
          border-radius: 10px;
          box-shadow: 0 4px 8px rgba(0,0,0,0.05);
          display: flex;
          flex-wrap: wrap;
          gap: 20px;
          align-items: center;
          justify-content: space-between;
        }

        .reading-item {
          display: flex;
          flex-direction: column;
          min-width: 100px;
        }

        .label {
          font-size: 0.9em;
          color: #888;
        }

        .value {
          font-size: 1.2em;
          font-weight: bold;
          color: #222;
        }

        @media (max-width: 600px) {
          .reading-card {
            flex-direction: column;
            align-items: flex-start;
          }
        }
      </style>
    </head>
    <body>
      <h1>Readings History</h1>
      <div class="subtitle-space"></div>

      <div class="button-container">
        <button class="button" onclick="window.location.href='/'">Home</button>
        <button class="button" onclick="loadReadings()">Refresh</button>
      </div>

      <div class="readings-container" id="readingsContainer"></div>

      <script>
        // Capitalize the first letter of the string
        function capitalizeFirstLetter(str) {
          return str.charAt(0).toUpperCase() + str.slice(1);
        }

        async function loadReadings() {
          try {
            const container = document.getElementById('readingsContainer');
            container.innerHTML = ''; // Clear current readings

            const response = await fetch('http://192.168.1.137:3000/history');
            const data = await response.json();

            if (data.length === 0) {
              container.innerHTML = '<p>No readings available.</p>';
              return;
            }

            data.forEach((reading) => {
              const card = document.createElement('div');
              card.className = 'reading-card';

              for (const key in reading) {
                const item = document.createElement('div');
                item.className = 'reading-item';

                const label = document.createElement('div');
                label.className = 'label';
                label.textContent = capitalizeFirstLetter(key); // Capitalize the key label

                const value = document.createElement('div');
                value.className = 'value';
                value.textContent = reading[key];

                item.appendChild(label);
                item.appendChild(value);
                card.appendChild(item);
              }

              container.appendChild(card);
            });
          } catch (error) {
            console.error('Error loading readings:', error);
            document.getElementById('readingsContainer').innerHTML = '<p>Error loading data.</p>';
          }
        }

        // Load initially
        loadReadings();
      </script>
    </body>
    </html>
)rawliteral";

  server.send(200, "text/html", html);
}


void setup() {
  // Start the Serial Monitor
  Serial.begin(9600);
  Serial.println();

  // Start the DS18B20 sensor
  Serial.println("Starting DHT11 sensor...");
  dht.begin();

  // Initialize readings
  lastCelciusTemperatureReading = dht.readTemperature();
  lastHumidityReading = dht.readHumidity();

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
  server.on("/reading", handleReading);
  server.on("/history", handleHistory);
  server.on("/allReadings", handleAllReadings);
  server.begin();
}

void loop() {
  // Update the temperature if needed
  unsigned long currentTime = millis();
  if (currentTime - lastReadingTime >= timeInterval) {
    // Get data from the DHT11 sensor
    lastCelciusTemperatureReading = dht.readTemperature();
    lastHumidityReading = dht.readHumidity();
    lastReadingTime = currentTime;

    // Create the payload as a JSON string
    String payload = "{\"temperature\": " + String(lastCelciusTemperatureReading) + ", \"humidity\": " + String(lastHumidityReading) + "}";

    // Send POST request with JSON payload
    if (httpClient.begin(wifiClient, POST_URL)) {
      httpClient.addHeader("Content-Type", "application/json");
      int httpResponseCode = httpClient.POST(payload);

      // Check the response
      if (httpResponseCode > 0) {
        Serial.println("Data sent successfully");
      } else {
        Serial.print("Error sending data. Response code: ");
        Serial.println(httpResponseCode);
      }

      httpClient.end();
    }
  }

  server.handleClient();
}