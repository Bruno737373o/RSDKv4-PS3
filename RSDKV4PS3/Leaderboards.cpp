#include "Leaderboards.hpp"
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/synchronization.h>
#include <cell/http.h>

char leaderboardURL[LEADERBOARD_URL_MAX] = "";
LeaderboardEntry_Online onlineLeaderboards[MAX_ONLINE_ENTRIES];
int onlineEntryCount = 0;
bool leaderboardLoading = false;
sys_lwmutex_t leaderboardMutex;

static sys_ppu_thread_t leaderboardThread;
static bool threadRunning = false;

struct LeaderboardRequest {
    int type; // 0 = fetch, 1 = submit
    int id;
    int score;
};

static LeaderboardRequest currentRequest;

#define HTTP_POOL_SIZE (64 * 1024)
static uint8_t httpPool[HTTP_POOL_SIZE] __attribute__((aligned(128)));

static void LeaderboardLoop(uint64_t arg)
{
    int res = cellHttpInit(httpPool, HTTP_POOL_SIZE);
    if (res < 0) {
        PrintLog("Leaderboards - cellHttpInit failed: 0x%08X", res);
        threadRunning = false;
        sys_ppu_thread_exit(0);
    }

    // Initialize HTTPS with 0 CA certs (trust all for simplicity in this context, 
    // though not recommended for production, it's often necessary on legacy consoles)
    cellHttpsInit(0, NULL);

    while (threadRunning) {
        bool shouldProcess = false;
        sys_lwmutex_lock(&leaderboardMutex, 0);
        shouldProcess = leaderboardLoading;
        sys_lwmutex_unlock(&leaderboardMutex, 0);

        if (shouldProcess) {
            CellHttpClientId clientId;
            res = cellHttpCreateClient(&clientId);
            if (res == 0) {
                cellHttpClientSetAutoRedirect(clientId, true);
                
                char fullURL[1024];
                sys_lwmutex_lock(&leaderboardMutex, 0);
                if (currentRequest.type == 0) { // Fetch
                    sprintf(fullURL, "%s?id=%d", leaderboardURL, currentRequest.id);
                } else { // Submit
                    char encodedName[128];
                    int j = 0;
                    for (int i = 0; playerName[i] && j < 120; i++) {
                        if (playerName[i] == ' ') {
                            encodedName[j++] = '%';
                            encodedName[j++] = '2';
                            encodedName[j++] = '0';
                        } else {
                            encodedName[j++] = playerName[i];
                        }
                    }
                    encodedName[j] = '\0';
                    sprintf(fullURL, "%s?id=%d&score=%d&name=%s", leaderboardURL, currentRequest.id, currentRequest.score, encodedName);
                }
                sys_lwmutex_unlock(&leaderboardMutex, 0);

                CellHttpUri uri;
                cellHttpUtilParseUri(&uri, fullURL, NULL, 0, NULL);

                CellHttpTransId transId;
                res = cellHttpCreateTransaction(&transId, clientId, "GET", &uri);
                if (res == 0) {
                    res = cellHttpSendRequest(transId, NULL, 0, NULL);
                    if (res == 0) {
                        int32_t statusCode;
                        cellHttpResponseGetStatusCode(transId, &statusCode);

                        if (statusCode == 200) {
                            char *responseBody = (char *)malloc(8192);
                            size_t recvd = 0;
                            cellHttpRecvResponse(transId, responseBody, 8191, &recvd);
                            responseBody[recvd] = 0;

                            if (currentRequest.type == 0) {
                                sys_lwmutex_lock(&leaderboardMutex, 0);
                                onlineEntryCount = 0;
                                char *line = strtok(responseBody, "\n");
                                while (line && onlineEntryCount < MAX_ONLINE_ENTRIES) {
                                    sscanf(line, "%d,%63[^,],%d", 
                                        &onlineLeaderboards[onlineEntryCount].rank,
                                        onlineLeaderboards[onlineEntryCount].name,
                                        &onlineLeaderboards[onlineEntryCount].score);
                                    onlineEntryCount++;
                                    line = strtok(NULL, "\n");
                                }
                                sys_lwmutex_unlock(&leaderboardMutex, 0);
                            }
                            free(responseBody);
                        } else {
                            PrintLog("Leaderboards - HTTP error: %d", statusCode);
                        }
                    }
                    cellHttpDestroyTransaction(transId);
                }
                cellHttpDestroyClient(clientId);
            }

            sys_lwmutex_lock(&leaderboardMutex, 0);
            leaderboardLoading = false;
            sys_lwmutex_unlock(&leaderboardMutex, 0);
        }
        sys_timer_usleep(100000);
    }

    cellHttpsEnd();
    cellHttpEnd();
    sys_ppu_thread_exit(0);
}

void InitLeaderboards()
{
    if (!threadRunning) {
        sys_lwmutex_attribute_t attr;
        sys_lwmutex_attribute_initialize(attr);
        if (sys_lwmutex_create(&leaderboardMutex, &attr) != CELL_OK) {
            PrintLog("Leaderboards - Failed to create mutex");
            return;
        }

        threadRunning = true;
        sys_ppu_thread_create(&leaderboardThread, LeaderboardLoop, 0, 100, 16384, SYS_PPU_THREAD_CREATE_JOINABLE, "LeaderboardThread");
    }
}

void ShutdownLeaderboards()
{
    if (threadRunning) {
        threadRunning = false;
        uint64_t exit_code;
        sys_ppu_thread_join(leaderboardThread, &exit_code);
        sys_lwmutex_destroy(&leaderboardMutex);
    }
}

void SubmitScore(int leaderboardID, int score)
{
    if (leaderboardURL[0] == '\0') return;
    sys_lwmutex_lock(&leaderboardMutex, 0);
    currentRequest.type = 1;
    currentRequest.id = leaderboardID;
    currentRequest.score = score;
    leaderboardLoading = true;
    sys_lwmutex_unlock(&leaderboardMutex, 0);
}

void FetchLeaderboard(int leaderboardID)
{
    if (leaderboardURL[0] == '\0') return;
    sys_lwmutex_lock(&leaderboardMutex, 0);
    currentRequest.type = 0;
    currentRequest.id = leaderboardID;
    leaderboardLoading = true;
    sys_lwmutex_unlock(&leaderboardMutex, 0);
}
