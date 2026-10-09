#include "NHL_API.h"
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api-web.nhle.com");

int TimeTillNow(String startTime){
  struct tm gameTime;
  strptime(startTime.c_str(), "%Y-%m-%dT%H:%M:%SZ", &gameTime);
  time_t gameEpoch = mktime(&gameTime);
  //Get current time.
  time_t currentEpoch;
  time(&currentEpoch);
  //Calculate difference and delay.
  double secondsToGame = std::abs(difftime(gameEpoch, currentEpoch));
  return (secondsToGame + 60) * 1000;
}

/*
Function to get amount of seconds until next game. Returns 0 if live game is found.
result = checkSchedule();
*/
int timeTilGame(String DateURL, String Team){
  String startTime;
  String startTime2;
  String gameState;
  client.get("/v1/club-schedule/{team}/week/{date}")
        .path("team", Team)
        .path("date", DateURL)
        .getBody("games.0.startTimeUTC", &startTime)
        .getBody("games.0.gameState", &gameState)
        .getBody("games.1.startTimeUTC", &startTime2);
  
  // Serial.println(gameState);
  if(gameState == "LIVE" || gameState == "CRIT"){
    return 0;
  } else if (gameState == "FINAL" || gameState == "OFF"){
    return TimeTillNow(startTime2);
  } else if (gameState == "FUT"){
    return TimeTillNow(startTime);
  } else if (gameState =="PRE"){
    return 30000;
  }
  Serial.println("Unknown game state. Retrying in 30 seconds.");
  return 30000;
}

//Function to find active game's ID.
String FindGameID(String DateURL, String Team) {
  String GameID;
  client.get("/v1/club-schedule/{team}/week/{date}")
        .path("team", Team)
        .path("date", DateURL)
        .getBody("games.0.id", &GameID);
  return GameID;
}

String FindTeamLocation(String Team, String GameID) {
  String teamLocation;
  String awayTeam;
  client.get("/v1/gamecenter/{gameID}/boxscore")
        .path("gameID", GameID)
        .getBody("awayTeam.abbrev", &awayTeam);
    if (awayTeam == Team){
      teamLocation = "awayTeam";
    } else {
      teamLocation = "homeTeam";
    }
    return teamLocation;
}

//Parses boxscore api to pull out the score based on the team locaiton inputed as argument 2.
int GetScore(String GameID, String teamLocation) {
  int homeScore;
  int awayScore;
  String gameState;
  client.get("/v1/gamecenter/{gameID}/boxscore")
        .path("gameID", GameID)
        .getBody("homeTeam.score", &homeScore)
        .getBody("awayTeam.score", &awayScore)
        .getBody("gameState", &gameState);
  if(gameState != "FINAL"){
    if(teamLocation == "awayTeam"){
      return awayScore ? awayScore : 0;
    } else {
      return homeScore ? homeScore : 0;
    }
  } else {
    return -1;
  }
}


//TODO: Split this into two functions, one for time and another for period number. Also include handling / displaying if intermission
//Parses boxscore api to pull out the time remaining and period as a string.
//  String result = checkGameStats();
String GetTimeRemaning(String GameID) {
  String timeRemaining;
  String currentPeriod;
  String gameState;
  client.get("/v1/gamecenter/{gameID}/boxscore")
        .path("gameID", GameID)
        .getBody("clock.timeRemaining", &timeRemaining)
        .getBody("periodDescriptor.number", &currentPeriod)
        .getBody("gameState", &gameState);
  if(gameState != "FINAL"){
    return "Period " + currentPeriod + "\n Time Remaining: " + timeRemaining;
  } else {
    return "Game finished";
  }
}