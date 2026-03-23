#ifndef LEADERBOARDS_H
#define LEADERBOARDS_H

#include "RetroEngine.hpp"
#include <sys/synchronization.h>

#define LEADERBOARD_URL_MAX 256
extern char leaderboardURL[LEADERBOARD_URL_MAX];

struct LeaderboardEntry_Online {
    char name[64];
    int score;
    int rank;
};

#define MAX_ONLINE_ENTRIES 20
extern LeaderboardEntry_Online onlineLeaderboards[MAX_ONLINE_ENTRIES];
extern int onlineEntryCount;
extern bool leaderboardLoading;
extern sys_lwmutex_t leaderboardMutex;

void InitLeaderboards();
void ShutdownLeaderboards();
void SubmitScore(int leaderboardID, int score);
void FetchLeaderboard(int leaderboardID);

#endif
