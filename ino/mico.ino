#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <vector>  // Include vector for managing multiple alarms

// WiFi Configuration
const char* ssid = "James";
const char* password = "pito1234";

// NTP Client Configuration
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // UTC-3 timezone, updates every 60 seconds

// Web Server Configuration
WiFiServer server(80);

// LCD Display Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); // Check address with I2C scanner
#define scl 22
#define sda 21

// Output Pin for Alarm Signal
#define alarmPin 13

// List of Alarm Times
std::vector<String> alarmTimes;

String urlDecode(String str) {
    String decodedString = "";
    char c;
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            String hex = str.substring(i + 1, i + 3);
            c = (char) strtol(hex.c_str(), NULL, 16);
            decodedString += c;
            i += 2; // Skip the next two characters
        } else {
            decodedString += str[i];
        }
    }
    return decodedString;
}

void setup() {
    Serial.begin(115200); // Start Serial Communication

    // I2C and LCD Initialization
    Wire.begin(sda, scl);
    lcd.init();
    lcd.backlight();
    lcd.print("Connecting...");

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.println("IP Address: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.print("Connected!");

    // Initialize NTP Client
    timeClient.begin();

    // Start Web Server
    server.begin();
    Serial.println("Web Server Started");

    // Configure Alarm Output Pin
    pinMode(alarmPin, OUTPUT);
}

void loop() {
    timeClient.update();
    String formattedTime = timeClient.getFormattedTime();

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(formattedTime);

    if (alarmTimes.empty()) {
        lcd.setCursor(0, 1);
        lcd.print("No alarms");
    } else {
        lcd.setCursor(0, 1);
        lcd.print("Next: " + alarmTimes[0]);
    }

    // Check Alarms
    for (const String& alarm : alarmTimes) {
        if (formattedTime == alarm) {
            Serial.println("ALARM! Time to wake up!");
            digitalWrite(alarmPin, HIGH);
            delay(5000);
            digitalWrite(alarmPin, LOW);
        }
    }

    // Handle Client Requests
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New Client Connected");
        String request = "";

        while (client.connected() && client.available()) {
            char c = client.read();
            request += c;
        }

        Serial.println("Request Received:\n" + request);

        // Handle CORS Preflight
        if (request.indexOf("OPTIONS") == 0) {
            handleCORS(client);
        } else if (request.indexOf("/set_alarm") >= 0) {
            handleSetAlarm(request, client);
        } else if (request.indexOf("/remove_alarm") >= 0) {
            handleRemoveAlarm(request, client);
        } else {
            sendBadRequest(client);
        }

        client.stop();
        Serial.println("Client Disconnected");
    }

    delay(1000);
}

// Function to Handle Setting Alarms
void handleSetAlarm(const String& request, WiFiClient& client) {
    // Add CORS headers
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
    client.println("Access-Control-Allow-Headers: Content-Type");

    int timeIndex = request.indexOf("time=") + 5; // Find the start of the time parameter
    int endIndex = request.indexOf(' ', timeIndex); // Find the end of the time parameter
    if (endIndex == -1) {
        endIndex = request.length(); // If there's no space, take the rest of the string
    }

    if (timeIndex > 0 && endIndex - timeIndex >= 5) { // Ensure we can get HH:MM
        String newAlarm = request.substring(timeIndex, endIndex); // Extract HH:MM
        newAlarm = urlDecode(newAlarm);  // Decode the URL-encoded string
        newAlarm += ":00"; // Append seconds to make it HH:MM:SS
        Serial.println("Received Alarm Time: " + newAlarm); // Log the received alarm time
        alarmTimes.push_back(newAlarm); // Add the new alarm to the vector

        // Log the entire alarmTimes vector for debugging
        Serial.print("Current Alarms: ");
        for (const String& alarm : alarmTimes) {
            Serial.print(alarm + " ");
        }
        Serial.println(); // New line for clarity

        // Respond to Client
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Alarm Set: " + newAlarm);
    } else {
        sendBadRequest(client);
    }
}

void handleRemoveAlarm(const String& request, WiFiClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();

    int timeIndex = request.indexOf("time=") + 5;
    if (timeIndex > 0 && timeIndex + 8 <= request.length()) {
        String alarmToRemove = request.substring(timeIndex, timeIndex + 8);
        alarmTimes.erase(std::remove(alarmTimes.begin(), alarmTimes.end(), alarmToRemove), alarmTimes.end());
        Serial.println("Alarm Removed: " + alarmToRemove);
        client.println("Alarm Removed: " + alarmToRemove);
    } else {
        client.println("Error: Invalid Time Format");
    }
}

// Function to Send a Bad Request Response
void sendBadRequest(WiFiClient& client) {
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println("Bad Request");
}

void handleCORS(WiFiClient& client) {
    client.println("HTTP/1.1 204 No Content");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();
}

