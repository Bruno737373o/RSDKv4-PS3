#ifndef NATIVE_LEADERBOARDSMENU_H
#define NATIVE_LEADERBOARDSMENU_H

#if !RETRO_USE_ORIGINAL_CODE
struct NativeEntity_LeaderboardsMenu : NativeEntityBase {
    NativeEntity_TextLabel *label;
    MeshInfo *meshPanel;
    MatrixF renderMatrix;
    MatrixF matrixTemp;
    float scale;
    byte backPressed;
    int state;
    float timer;
    int selection;
};

void LeaderboardsMenu_Create(void *objPtr);
void LeaderboardsMenu_Main(void *objPtr);
#endif

#endif // !NATIVE_LEADERBOARDSMENU_H
