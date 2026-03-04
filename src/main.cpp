#include <Arduino.h>
#include <WiFi.h>
#include <NHL_API.h>



// WIFI CREDENTIALS
const char* ssid = "IFoundYourFetlifePage";
const char* password = "CalorieAcceptorSpaces835";

//Hockey Constants
String GameID, teamLocation;
const int RedLED = 5;
int OldScore;
String Team = "COL";


//Time Constants
const char* NTP_SERVER = "pool.ntp.org";
int UTC_OFFSET = 0;
int UTC_OFFSET_DST = 0;
struct tm timeinfo;


//Function declarations.
void Score(int flashAmount);
void sendHTML();

//Setup Function
void setup() {
  //Start Serial Monitor and wait a few seconds to allow connection
  Serial.begin(115200);
  pinMode(RedLED, OUTPUT);
  delay(3000);

  //Connect to WiFi
  WiFi.begin(ssid, password, 6);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.print("\nConnected! IP=");
  Serial.println(WiFi.localIP());

  //Initialize time
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println("Time Set!");
  OldScore = 0;
}

void loop() {
  //Check the schedule for the week, passing along today's date to prevent dealing with redirecting.
  char DateURL[10];
  int today = sprintf(DateURL, "%04d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday - 1);
  String scheduleResult = checkSchedule(String(DateURL), Team);
  //Check how long until the next game.
  int timeTilNextGame = timeTilGame(scheduleResult);
  if (timeTilNextGame > 0){
    delay(timeTilNextGame);
    return;
  }
  //If there's a game live now, find it's ID to pull the game's boxscore
  GameID = FindGameID(scheduleResult);
  String gameResult = checkGameStats(GameID);
  //Set the team's location to ensure you're watching to correct score.
  teamLocation = FindTeamLocation(gameResult, Team);
  if(!GameID || !teamLocation){
    Serial.print("Something fucked up");
    return;
  }
  //Initialize newScore variable. May as well populate it using the same API call we used to get the team's location.
  int newScore = GetScore(gameResult, teamLocation);

  //IT'S HOCKEY TIME. Watch the score and react when needed. GetScore() will return -1 when the game is over.
  while (newScore != -1){
    //Call the API
    gameResult = checkGameStats(GameID);
    //Set the score variable.
    newScore = GetScore(gameResult, teamLocation);
    if (newScore == OldScore){
      //Delay to reduce API calls.
      delay(5000);
      continue;
    } else if(newScore > OldScore){
      //We scored. Time to react.
      //TODO: Add variable wait time to account for stream delay. Add input on website to change the amount of time.
      delay(50000);
      Serial.println("Score Changed!");
      Serial.print(GetTimeRemaning(gameResult));
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

void sendHTML(){
  // server.send(200, "text/html",
  //   "<!DOCTYPE html>"
  //   "<html>"
  //   "<body>"
  //   "<h2>ESP32 LED</h2>"
  //   "<p><a href='/on'>ON</a></p>"
  //   "<p><a href='/off'>OFF</a></p>"
  //   // "<p>Status: " + String(Team) + "</p>"
  //   "</body>"
  //   "</html>"
  // );
  //palceholder for future configuration page.
  //If no wifi credentials are stored, host a configuration page.
  //Add input fields for wifi ssid and password.
  //add input fields for team selection.
  //add debugging information, such as next game time, current score, etc.
}