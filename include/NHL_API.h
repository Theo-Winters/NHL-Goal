#ifndef NHL_API_H
#define NHL_API_H
#include <Arduino.h>


String checkSchedule(String DateURL, String Team);
int timeTilGame(String result);
String FindGameID(String result);
String FindTeamLocation(String result, String Team);
String checkGameStats(String GameID);
int GetScore(String result, String teamLocation);
String GetTimeRemaning(String result);

#endif