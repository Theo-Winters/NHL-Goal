#include <Arduino.h> //Is this needed?
#include <stdio.h> //This is needed for sprintf to work.
#include <time.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

#include <NHL_API.h>
#include "secrets.h"

//WebServer for WebSerial
AsyncWebServer server(80);

//SET YOUR TEAM HERE. Use the 3 letter abbreviation for your team based on NHL's API. Example: Colorado Avalanche = "COL" (https://github.com/Zmalski/NHL-API-Reference#team-information)
String Team = "COL";

//Variables
String GameID, teamLocation;
int OldScore;
int newScore;
int StreamOffset = 50000;

//Pin Constants - GPIO Pin that's connected to the gate of the MOSFET.
const int RedLED = 5;

//Time Constants
const char* NTP_SERVER = "pool.ntp.org";
int UTC_OFFSET = 0;
int UTC_OFFSET_DST = 0;
struct tm timeinfo;

//Function declarations.
void Score(int flashAmount);
void NonBlockDelay(uint32_t ms);
String timeResponse();

//Setup Function
void setup() {
  //Start Serial Monitor and wait a few seconds to allow connection
  Serial.begin(115200);
  pinMode(RedLED, OUTPUT);
  digitalWrite(RedLED, LOW);
  
  //Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (attempts > 20){
      Serial.println("Failed to connect to WiFi. Restarting.");
      ESP.restart();
    }
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.print("\nConnected! IP=");
  Serial.println(WiFi.localIP());
  
  //Start WebSerial
  WebSerial.begin(&server);
 
  // Attach callback to handle incoming messages from the WebSerial client
  WebSerial.onMessage([](uint8_t *data, size_t len) {
    Serial.printf("Received %lu bytes from WebSerial: ", len);
    Serial.write(data, len);
    Serial.println();
    WebSerial.println("Received Data...");
    String d = "";
    for(size_t i = 0; i < len; i++){
      d += char(data[i]);
    }
    if(d == "reset"){
      WebSerial.println("Resetting...");
      ESP.restart();
    } else if(d == "score"){
      WebSerial.println("Scoring...");
      Score(1);
    } else if (d.toInt() > 0){
      StreamOffset = d.toInt() * 1000;
      WebSerial.print("Stream Offset set to: ");
      WebSerial.print(StreamOffset/1000);
      WebSerial.println(" seconds.");
    } else if (d == "time"){
      WebSerial.println(timeResponse());
    }
    
    else {
      WebSerial.println("Unknown Command.");
      WebSerial.println(d);
    }
  });
  
  // Start AsyncWebServer
  server.begin();
  delay(3000);
  
  //Initialize time
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time. Restarting.");
    ESP.restart();
    return;
  }
  Serial.println("Time Set!");
  OldScore = 0;
}

void loop() {
  if(GameID == ""){
    //Check the schedule for the week, passing along today's date to prevent dealing with redirecting.
    time_t raw_time = mktime(&timeinfo);
    raw_time -= 86400; 
    struct tm *yesterday = localtime(&raw_time);
    char DateURL[11];
    sprintf(DateURL, "%04d-%02d-%02d", yesterday->tm_year + 1900, yesterday->tm_mon + 1, yesterday->tm_mday);
    int timeTilNextGame = timeTilGame(DateURL, Team);
    if (timeTilNextGame > 0){
      WebSerial.print("No game live. Sleeping for ");
      WebSerial.println(timeTilNextGame);
      //TODO: Replace Delay with non blocking delay to allow WebSerial to function while waiting for next game.
      NonBlockDelay(timeTilNextGame);
      return;
    }
    //If there's a game live now, find it's ID to pull the game's boxscore
    GameID = FindGameID(DateURL, Team);
    //Set the team's location to ensure you're watching to correct score.
    teamLocation = FindTeamLocation(Team, GameID);
    if(!GameID || !teamLocation){
      WebSerial.print("Something fucked up");
      return;
    }
    WebSerial.print("Game ID: ");
    WebSerial.println(GameID);
    WebSerial.print("Team Location: ");
    WebSerial.println(teamLocation);
  }
  //If GameID is set, then the game is live and we can watch the score. TODO: Remove While loop
  newScore = GetScore(GameID, teamLocation);
  if (newScore == OldScore){
    //Delay to reduce API calls.
    delay(1000);
  } else if(newScore > OldScore){
    //We scored. Time to react.
    String timeRemaining = GetTimeRemaining(GameID);
    NonBlockDelay(StreamOffset);
    WebSerial.println("Score Changed!");
    WebSerial.print("New Score: ");
    WebSerial.println(newScore);
    WebSerial.print(timeRemaining);
    Score(1);
    //Set this just in case a goal was scored, and then revoked.
    OldScore = newScore;
  }
  //If newScore is -1, then the game is over. Reset all the variables and start checking for the next game.
  if(newScore == -1){
    WebSerial.println("Game Over. Resetting variables.");
    GameID = "";
    teamLocation = "";
    OldScore = 0;
    newScore = 0;
  }
  WebSerial.loop();
}

void Score(int flashAmount){
      digitalWrite(RedLED, HIGH);
      NonBlockDelay(5000);
      digitalWrite(RedLED, LOW); 
      //TODO: Add buzzer or other notification method.
      //TODO: investigate spinning light.
}

void NonBlockDelay(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    WebSerial.loop(); // Allow WebSerial to process incoming messages
    delay(1); // Small delay to prevent blocking
  }
}

String timeResponse(){
  if (GameID != ""){
    String timeRemaining = GetTimeRemaining(GameID);
    return timeRemaining;
  } else {
    return "No game live.";
  }
}