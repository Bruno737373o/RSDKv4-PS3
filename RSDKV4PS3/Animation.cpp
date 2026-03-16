#include "RetroEngine.hpp"

AnimationFile animationFileList[ANIFILE_COUNT];
int animationFileCount = 0;

SpriteFrame scriptFrames[SPRITEFRAME_COUNT];
int scriptFrameCount = 0;

SpriteFrame animFrames[SPRITEFRAME_COUNT];
int animFrameCount = 0;
SpriteAnimation animationList[ANIMATION_COUNT];
int animationCount = 0;
Hitbox hitboxList[HITBOX_COUNT];
int hitboxCount = 0;

void LoadAnimationFile(char *filePath)
{
    FileInfo info;
    if (LoadFile(filePath, &info)) {
        size_t size = (size_t)info.vfileSize;
        byte *fileData = (byte *)malloc(size);
        if (!fileData) {
            CloseFile();
            return;
        }
        FileRead(fileData, (int)size);
        CloseFile();

        byte *ptr = fileData;
        char strBuf[0x21];
        byte sheetIDs[0x18];
        sheetIDs[0] = 0;

        byte sheetCount = *ptr++;

        for (int s = 0; s < sheetCount; ++s) {
            byte len = *ptr++;
            if (len) {
                int i = 0;
                for (; i < len; ++i) strBuf[i] = *ptr++;
                strBuf[i] = 0;
                sheetIDs[s] = AddGraphicsFile(strBuf);
            }
        }

        byte animCount = *ptr++;
        AnimationFile *animFile = &animationFileList[animationFileCount];
        animFile->animCount     = animCount;
        animFile->aniListOffset = animationCount;

        for (int a = 0; a < animCount; ++a) {
            SpriteAnimation *anim = &animationList[animationCount++];
            anim->frameListOffset = animFrameCount;
            byte len = *ptr++;
            for (int i = 0; i < len; ++i) anim->name[i] = *ptr++;
            anim->name[len] = 0;
            anim->frameCount = *ptr++;
            anim->speed = *ptr++;
            anim->loopPoint = *ptr++;
            anim->rotationStyle = *ptr++;

            for (int j = 0; j < anim->frameCount; ++j) {
                SpriteFrame *frame = &animFrames[animFrameCount++];
                frame->sheetID = sheetIDs[*ptr++];
                frame->hitboxID = *ptr++;
                frame->sprX = *ptr++;
                frame->sprY = *ptr++;
                frame->width = *ptr++;
                frame->height = *ptr++;
                frame->pivotX = (sbyte)*ptr++;
                frame->pivotY = (sbyte)*ptr++;
            }

            // 90 Degree (Extra rotation Frames) rotation
            if (anim->rotationStyle == ROTSTYLE_STATICFRAMES)
                anim->frameCount >>= 1;
        }

        animFile->hitboxListOffset = hitboxCount;
        byte hitCount = *ptr++;
        for (int i = 0; i < hitCount; ++i) {
            Hitbox *hitbox = &hitboxList[hitboxCount++];
            for (int d = 0; d < HITBOX_DIR_COUNT; ++d) {
                hitbox->left[d] = (sbyte)*ptr++;
                hitbox->top[d] = (sbyte)*ptr++;
                hitbox->right[d] = (sbyte)*ptr++;
                hitbox->bottom[d] = (sbyte)*ptr++;
            }
        }

        free(fileData);
    }
}
void ClearAnimationData()
{
    for (int f = 0; f < SPRITEFRAME_COUNT; ++f) MEM_ZERO(scriptFrames[f]);
    for (int f = 0; f < SPRITEFRAME_COUNT; ++f) MEM_ZERO(animFrames[f]);
    for (int h = 0; h < HITBOX_COUNT; ++h) MEM_ZERO(hitboxList[h]);
    for (int a = 0; a < ANIMATION_COUNT; ++a) MEM_ZERO(animationList[a]);
    for (int a = 0; a < ANIFILE_COUNT; ++a) MEM_ZERO(animationFileList[a]);

    scriptFrameCount   = 0;
    animFrameCount     = 0;
    animationCount     = 0;
    animationFileCount = 0;
    hitboxCount        = 0;
}

AnimationFile *AddAnimationFile(char *filePath)
{
    char path[0x80];
    StrCopy(path, "Data/Animations/");
    StrAdd(path, filePath);

    for (int a = 0; a < ANIFILE_COUNT; ++a) {
        if (StrLength(animationFileList[a].fileName) <= 0) {
            StrCopy(animationFileList[a].fileName, filePath);
            LoadAnimationFile(path);
            ++animationFileCount;
            return &animationFileList[a];
        }
        if (StrComp(animationFileList[a].fileName, filePath))
            return &animationFileList[a];
    }
    return NULL;
}

void ProcessObjectAnimation(void *objScr, void *ent)
{
    ObjectScript *objectScript = (ObjectScript *)objScr;
    Entity *entity             = (Entity *)ent;
    SpriteAnimation *sprAnim   = &animationList[objectScript->animFile->aniListOffset + entity->animation];

    if (entity->animationSpeed <= 0) {
        entity->animationTimer += sprAnim->speed;
    }
    else {
        if (entity->animationSpeed > 0xF0)
            entity->animationSpeed = 0xF0;
        entity->animationTimer += entity->animationSpeed;
    }

    if (entity->animation != entity->prevAnimation) {
        entity->prevAnimation  = entity->animation;
        entity->frame          = 0;
        entity->animationTimer = 0;
        entity->animationSpeed = 0;
    }

    if (entity->animationTimer >= 0xF0) {
        entity->animationTimer -= 0xF0;
        ++entity->frame;
    }

    if (entity->frame >= sprAnim->frameCount)
        entity->frame = sprAnim->loopPoint;
}