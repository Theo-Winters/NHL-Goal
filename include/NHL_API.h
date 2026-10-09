#ifndef NHL_API_H
#define NHL_API_H
#include <Arduino.h>


int timeTilGame(String DateURL, String Team);
int TimeTillNow(String startTime);
String FindGameID(String DateURL, String Team);
String FindTeamLocation(String Team, String GameID);
int GetScore(String GameID, String teamLocation);
String GetTimeRemaning(String GameID);
#endif