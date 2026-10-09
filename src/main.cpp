#include <Arduino.h>
#include <WiFi.h>
#include <NHL_API.h>
#include <stdio.h>
#include <time.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>


// WIFI CREDENTIALS
const char* ssid = "Underground_AI_Data_Center";
const char* password = "yourmomst!ts";

AsyncWebServer server(80);

//Hockey Constants
String GameID, teamLocation;
const int RedLED = 5;
int OldScore;
String Team = "COL";
int StreamOffset = 50000;

//Time Constants
const char* NTP_SERVER = "pool.ntp.org";
int UTC_OFFSET = 0;
int UTC_OFFSET_DST = 0;
struct tm timeinfo;

//Function declarations.
void Score(int flashAmount);

//Setup Function
void setup() {
  //Start Serial Monitor and wait a few seconds to allow connection
  Serial.begin(115200);
  pinMode(RedLED, OUTPUT);
  delay(3000);

  //Connect to WiFi
  WiFi.begin(ssid, password, 6);
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
    WebSerial.println(d);
  });
 
  // Start AsyncWebServer
  server.begin();

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
  WebSerial.loop();
  //Check the schedule for the week, passing along today's date to prevent dealing with redirecting.
  time_t raw_time = mktime(&timeinfo);
  raw_time -= 86400; 
  struct tm *yesterday = localtime(&raw_time);
  char DateURL[11];
  sprintf(DateURL, "%04d-%02d-%02d", yesterday->tm_year + 1900, yesterday->tm_mon + 1, yesterday->tm_mday);
  int timeTilNextGame = timeTilGame(DateURL, Team);
  if (timeTilNextGame > 0){
    delay(timeTilNextGame);
    return;
  }
  //If there's a game live now, find it's ID to pull the game's boxscore
  GameID = FindGameID(DateURL, Team);
  //Set the team's location to ensure you're watching to correct score.
  teamLocation = FindTeamLocation(Team, GameID);
  if(!GameID || !teamLocation){
    Serial.print("Something fucked up");
    return;
  }
  //Initialize newScore variable.
  int newScore = GetScore(GameID, teamLocation);

  //IT'S HOCKEY TIME. Watch the score and react when needed. GetScore() will return -1 when the game is over.
  while (newScore != -1){
    newScore = GetScore(GameID, teamLocation);
    if (newScore == OldScore){
      //Delay to reduce API calls.
      delay(1000);
      continue;
    } else if(newScore > OldScore){
      //We scored. Time to react.
      //TODO: Add variable wait time to account for stream delay. Add input on website to change the amount of time.
      delay(StreamOffset);
      Serial.println("Score Changed!");
      Serial.print(GetTimeRemaning(GameID));
      Score(1);
    }
    //Set this just in case a goal was scored, and then revoked.
    OldScore = newScore;
  }
  //Reset the stats in preperation of the next game.
  OldScore = 0;
  GameID = "";
  teamLocation = "";
}

void Score(int flashAmount){
      for(int i = 0; i < flashAmount; i++){
        digitalWrite(RedLED, HIGH);
        delay(5000);
        digitalWrite(RedLED, LOW); 
      }
      //TODO: Add buzzer or other notification method.
      //TODO: investigate spinning light.
}