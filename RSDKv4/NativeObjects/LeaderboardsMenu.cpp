#include "RetroEngine.hpp"
#include "Leaderboards.hpp"

#if !RETRO_USE_ORIGINAL_CODE
void LeaderboardsMenu_Create(void *objPtr)
{
    RSDK_THIS(LeaderboardsMenu);

    self->label                  = CREATE_ENTITY(TextLabel);
    self->label->useRenderMatrix = true;
    self->label->fontID          = FONT_HEADING;
    self->label->scale           = 0.2;
    self->label->alpha           = 256;
    self->label->x               = -144.0;
    self->label->y               = 100.0;
    self->label->z               = 16.0;
    self->label->state           = TEXTLABEL_STATE_IDLE;
    SetStringToFont(self->label->text, strLeaderboards, FONT_HEADING);

    self->scale      = 0;

    self->meshPanel = LoadMesh("Data/Game/Models/Panel.bin", -1);
    SetMeshVertexColors(self->meshPanel, 0, 0, 0, 0xC0);

    self->selection = 0; // Current leaderboard ID
    FetchLeaderboard(self->selection);
}

void LeaderboardsMenu_Main(void *objPtr)
{
    RSDK_THIS(LeaderboardsMenu);

    switch (self->state) {
        case 0: { // Opening
            self->scale = fminf(self->scale + ((1.05 - self->scale) / ((60.0 * Engine.deltaTime) * 8.0)), 1.0f);

            NewRenderState();
            MatrixScaleXYZF(&self->renderMatrix, self->scale, self->scale, 1.0);
            MatrixTranslateXYZF(&self->matrixTemp, 0.0, 0, 160.0);
            MatrixMultiplyF(&self->renderMatrix, &self->matrixTemp);
            SetRenderMatrix(&self->renderMatrix);

            memcpy(&self->label->renderMatrix, &self->renderMatrix, sizeof(MatrixF));

            self->timer += Engine.deltaTime;
            if (self->timer > 0.5) {
                self->timer = 0.0;
                self->state = 1;
            }
            break;
        }
        case 1: { // Main Loop
            CheckKeyPress(&keyPress);
            SetRenderMatrix(&self->renderMatrix);

            if (keyPress.B) {
                PlaySfxByName("Menu Back", false);
                self->state = 2;
                self->timer = 0;
            }

            // Render Leaderboard entries
            if (leaderboardLoading) {
                ushort text[32];
                SetStringToFont8(text, "LOADING...", FONT_LABEL);
                RenderText(text, FONT_LABEL, 0, 0, 16.0, 0.2, 255);
            } else {
                float y = 40.0f;
                int count = 0;
                
                // Get local copy to minimize lock time
                LeaderboardEntry_Online localEntries[MAX_ONLINE_ENTRIES];

                sys_lwmutex_lock(&leaderboardMutex, 0);
                count = onlineEntryCount;
                memcpy(localEntries, onlineLeaderboards, sizeof(localEntries));
                sys_lwmutex_unlock(&leaderboardMutex, 0);

                for (int i = 0; i < count; ++i) {
                    char buffer[128];
                    ushort text[128];
                    sprintf(buffer, "%d. %s - %d", localEntries[i].rank, localEntries[i].name, localEntries[i].score);
                    SetStringToFont8(text, buffer, FONT_LABEL);
                    RenderText(text, FONT_LABEL, -120.0, y, 16.0, 0.1, 255);
                    y -= 15.0f;
                }
                if (count == 0) {
                    ushort text[32];
                    SetStringToFont8(text, "NO DATA", FONT_LABEL);
                    RenderText(text, FONT_LABEL, 0, 0, 16.0, 0.15, 255);
                }
            }
            break;
        }
        case 2: { // Closing
            self->scale = fmaxf(self->scale + ((-1.0f - self->scale) / ((Engine.deltaTime * 60.0) * 8.0)), 0.0);

            NewRenderState();
            MatrixScaleXYZF(&self->renderMatrix, self->scale, self->scale, 1.0);
            MatrixTranslateXYZF(&self->matrixTemp, 0.0, 0, 160.0);
            MatrixMultiplyF(&self->renderMatrix, &self->matrixTemp);
            SetRenderMatrix(&self->renderMatrix);

            memcpy(&self->label->renderMatrix, &self->renderMatrix, sizeof(MatrixF));

            self->timer += Engine.deltaTime;
            if (self->timer > 0.5) {
                RemoveNativeObject(self->label);
                RemoveNativeObject(self);
                return;
            }
            break;
        }
    }

    RenderMesh(self->meshPanel, MESH_COLORS, false);
}
#endif
