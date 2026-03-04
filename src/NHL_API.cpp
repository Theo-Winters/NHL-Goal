#include "NHL_API.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>



//Return the weekly schedule for specified team. Checks using today's date.
String checkSchedule(String DateURL, String Team){
  String GameIDURL = "https://api-web.nhle.com/v1/club-schedule/"+ Team +"/week/" + DateURL;
  // Serial.print(GameIDURL);
  HTTPClient http;
  http.useHTTP10(true);
  http.begin(GameIDURL);
  http.GET();
  String result = http.getString();
  http.end();
  return result;
}

/*
Function to get amount of seconds until next game. Returns 0 if live game is found.
result = checkSchedule();
*/
int timeTilGame(String result){
  JsonDocument doc;
  deserializeJson(doc, result);
  
  for (JsonObject game : doc["games"].as<JsonArray>()) {
    //Check for the upcoming game.
    const String gameState = game["gameState"].as<String>();
    if(gameState == "LIVE" || gameState == "CRIT"){
      return 0;
    } else if (gameState == "FUT"){
      //Get game time.
      const String gameTimeStr = game["startTimeUTC"].as<String>();
      Serial.print("Next game at: ");
      Serial.println(gameTimeStr);
      struct tm gameTime;
      strptime(gameTimeStr.c_str(), "%Y-%m-%dT%H:%M:%SZ", &gameTime);
      time_t gameEpoch = mktime(&gameTime);
      //Get current time.
      time_t currentEpoch;
      time(&currentEpoch);
      //Calculate difference and delay.
      double secondsToGame = std::abs(difftime(gameEpoch, currentEpoch));
      Serial.print("Sleeping for ");
      Serial.print(secondsToGame);
      Serial.println(" seconds until next game.");
      // delay((secondsToGame + 60) * 1000); //Add 10 seconds buffer
      return (secondsToGame + 60) * 1000;
    } else if (gameState == "PRE"){
      // delay(30000);
      Serial.println("Game in PRE. Waiting for 30 seconds.");
      return 30000;
    } else {
      continue;
    }
  }
  Serial.println("Something went wrong. Trying again in 30 seconds.");
  return 30000;
}

//TODO: Update to bool, returning true if a live game is found and a GameID is set.
//Function to find active game's ID. If no live games, it will check the above function for how long until the next game.
//result = checkSchedule();
String FindGameID(String result) {
  String GameID = "";
  JsonDocument doc;
  deserializeJson(doc, result);
  for (JsonObject game : doc["games"].as<JsonArray>()) {
    //Check for the live game.
    const String gameState = game["gameState"].as<String>();
    if (gameState == "LIVE"|| gameState == "CRIT")
    {
      //Set game ID variable.
      GameID = game["id"].as<String>();
      Serial.print("Game ID: ");
      Serial.println(GameID);
      return GameID;
    }  
  }
  Serial.println("No live game found.");
  return GameID;
}

String FindTeamLocation(String result, String Team) {
    JsonDocument doc;
    deserializeJson(doc, result);
    String teamLocation;
    if (doc["awayTeam"]["abbrev"].as<String>() == Team){
      teamLocation = "awayTeam";
    } else {
      teamLocation = "homeTeam";
    }
    Serial.print("Team Location: ");
    Serial.println(teamLocation);
    return teamLocation;
}

//Checks the boxscore api for the specified game and returns the JSON data as a string. 
String checkGameStats(String GameID){
  HTTPClient http;
  String ScoreURL = "https://api-web.nhle.com/v1/gamecenter/" + GameID + "/boxscore";
  http.useHTTP10(true);
  http.begin(ScoreURL);
  http.GET();
  String result = http.getString();
  http.end();
  return result;
}


//TODO:Add argument for passing JSON data from Check game stats, instead of calling it.
//Parses boxscore api to pull out the score based on the team locaiton inputed as argument 1.
//String result = checkGameStats();
int GetScore(String result, String teamLocation) {
  JsonDocument doc;
  deserializeJson(doc, result);
  if(doc["gameState"] == "LIVE" || doc["gameState"] == "CRIT"){
    return doc[teamLocation]["score"] ? doc[teamLocation]["score"].as<int>() : 0;
  } else {
    Serial.print("Game has finished.");
    return -1;
  }
}


//TODO: Split this into two functions, one for time and another for period number. Also include handling / displaying if intermission
//Parses boxscore api to pull out the time remaining and period as a string.
//  String result = checkGameStats();
String GetTimeRemaning(String result) {
  JsonDocument doc;
  deserializeJson(doc, result);
  if(doc["gameState"] == "LIVE" || doc["gameState"] == "CRIT"){
    String periodTimeRemaining = doc["clock"]["timeRemaining"].as<String>();
    String PeriodNumber = doc["periodDescriptor"]["number"].as<String>();
    return "Period " + PeriodNumber + "\n Time Remaining: " + periodTimeRemaining;
  } else {
    return "Game finished";
  }
}