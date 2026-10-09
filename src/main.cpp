#include <Arduino.h>
#include <WiFi.h>
#include <NHL_API.h>
#include <stdio.h>
#include <time.h>


// WIFI CREDENTIALS
const char* ssid = "Underground_AI_Data_Center";
const char* password = "yourmomst!ts";


//Hockey Constants
String GameID, teamLocation;
const int RedLED = 5;
int OldScore;
int newScore;
String Team = "COL";
int StreamOffset = 80000;

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
  }
  //Initialize newScore variable.
  newScore = GetScore(GameID, teamLocation);
  
  //If GameID is set, then the game is live and we can watch the score. TODO: Remove While loop
  newScore = GetScore(GameID, teamLocation);
  if (newScore == OldScore){
    //Delay to reduce API calls.
    delay(1000);
    return;
  } else if(newScore > OldScore){
    //We scored. Time to react.
    String timeRemaining = GetTimeRemaning(GameID);
    delay(StreamOffset);
    Serial.println("Score Changed!");
    Serial.print("New Score: ");
    Serial.println(newScore);
    Serial.print(timeRemaining);
    Score(1);
    //Set this just in case a goal was scored, and then revoked.
    OldScore = newScore;
  }
  //If newScore is -1, then the game is over. Reset all the variables and start checking for the next game.
  if(newScore == -1){
    Serial.println("Game Over. Resetting variables.");
    GameID = "";
    teamLocation = "";
    OldScore = 0;
    newScore = 0;
  }
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