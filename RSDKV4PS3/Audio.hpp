#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>
#include <string.h>

#define TRACK_COUNT (0x10)
#define SFX_COUNT   (0x100)
#if !RETRO_USE_ORIGINAL_CODE
#define CHANNEL_COUNT (0x10) // 4 in the original, 16 for convenience
#else
#define CHANNEL_COUNT (0x4)
#endif

#define MAX_VOLUME (100)

#define MUSBUFFER_SIZE   (0x200000)
#define STREAMFILE_COUNT (2)

#define MIX_BUFFER_SAMPLES (256)

#if RETRO_USING_SDL1 || RETRO_USING_SDL2

#define LockAudioDevice()   SDL_LockAudio()
#define UnlockAudioDevice() SDL_UnlockAudio()

#elif RETRO_PLATFORM == RETRO_PS3

#include <sys/synchronization.h>
#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
extern sys_lwmutex_t audioMutex;
#define LockAudioDevice()   sys_lwmutex_lock(&audioMutex, 0xFFFFFFFF)
#define UnlockAudioDevice() sys_lwmutex_unlock(&audioMutex)

#ifndef RETROENGINE_H
// Standard types and macros for Audio.hpp if RetroEngine.hpp isn't included yet
#ifndef BYTE_DEFINED
#define BYTE_DEFINED
typedef unsigned char byte;
#endif
typedef signed char sbyte;
typedef unsigned short ushort;
typedef unsigned int uint;

typedef signed short Sint16;
typedef signed int Sint32;
typedef signed long long Sint64;
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long long Uint64;

#define MEM_ZERO(x)  memset(&(x), 0, sizeof((x)))
#define MEM_ZEROP(x) memset((x), 0, sizeof(*(x)))
#endif

#else
#define LockAudioDevice()   ;
#define UnlockAudioDevice() ;
#endif

struct TrackInfo {
    char fileName[0x40];
    bool trackLoop;
    uint loopPoint;
};

#if RETRO_PLATFORM == RETRO_PS3
struct __attribute__((aligned(16))) StreamInfo {
    Sint16 buffer[MIX_BUFFER_SAMPLES]; // Align buffer at the start
    OggVorbis_File vorbisFile;
    int vorbBitstream;
    bool trackLoop;
    uint loopPoint;
    bool loaded;
    byte padding[7]; // Ensure 16-byte alignment of the next element in an array
};

struct __attribute__((aligned(16))) SFXInfo {
    char name[0x40];
    Sint16 *buffer;
    size_t length;
    bool loaded;
    byte padding[7];
};

struct __attribute__((aligned(16))) ChannelInfo {
    size_t sampleLength;
    Sint16 *samplePtr;
    int sfxID;
    byte loopSFX;
    sbyte pan;
    byte padding[6];
};

struct __attribute__((aligned(16))) StreamFile {
    byte buffer[MUSBUFFER_SIZE];
    int fileSize;
    int filePos;
    byte padding[8];
};
#else
struct StreamInfo {
    OggVorbis_File vorbisFile;
    int vorbBitstream;
#if RETRO_USING_SDL1
    SDL_AudioSpec spec;
#endif
#if RETRO_USING_SDL2
    SDL_AudioStream *stream;
#endif
    Sint16 buffer[MIX_BUFFER_SAMPLES];
    bool trackLoop;
    uint loopPoint;
    bool loaded;
};

struct SFXInfo {
    char name[0x40];
    Sint16 *buffer;
    size_t length;
    bool loaded;
};

struct ChannelInfo {
    size_t sampleLength;
    Sint16 *samplePtr;
    int sfxID;
    byte loopSFX;
    sbyte pan;
};

struct StreamFile {
    byte buffer[MUSBUFFER_SIZE];
    int fileSize;
    int filePos;
};
#endif

enum MusicStatuses {
    MUSIC_STOPPED = 0,
    MUSIC_PLAYING = 1,
    MUSIC_PAUSED  = 2,
    MUSIC_LOADING = 3,
    MUSIC_READY   = 4,
};

extern int globalSFXCount;
extern int stageSFXCount;

extern int masterVolume;
extern int trackID;
extern int sfxVolume;
extern int bgmVolume;
extern bool audioEnabled;

extern bool musicEnabled;
extern int musicStatus;
extern int musicStartPos;
extern int musicPosition;
extern int musicRatio;
extern TrackInfo musicTracks[TRACK_COUNT];

extern int currentStreamIndex;
#if RETRO_PLATFORM == RETRO_PS3
extern StreamFile streamFile[STREAMFILE_COUNT] __attribute__((aligned(16)));
extern StreamInfo streamInfo[STREAMFILE_COUNT] __attribute__((aligned(16)));
#else
extern StreamFile streamFile[STREAMFILE_COUNT];
extern StreamInfo streamInfo[STREAMFILE_COUNT];
#endif
extern StreamFile *streamFilePtr;
extern StreamInfo *streamInfoPtr;

#if RETRO_PLATFORM == RETRO_PS3
extern SFXInfo sfxList[SFX_COUNT] __attribute__((aligned(16)));
#else
extern SFXInfo sfxList[SFX_COUNT];
#endif
extern char sfxNames[SFX_COUNT][0x40];

#if RETRO_PLATFORM == RETRO_PS3
extern ChannelInfo sfxChannels[CHANNEL_COUNT] __attribute__((aligned(16)));
#else
extern ChannelInfo sfxChannels[CHANNEL_COUNT];
#endif

#if RETRO_USING_SDL1 || RETRO_USING_SDL2
extern SDL_AudioSpec audioDeviceFormat;
#endif

int InitAudioPlayback();
void LoadGlobalSfx();

#if !RETRO_USE_ORIGINAL_CODE
// These functions did exist, but with different signatures
void ProcessMusicStream(Sint32 *stream, size_t bytes_wanted);
#if RETRO_USING_SDL1 || RETRO_USING_SDL2
void ProcessAudioPlayback(void *data, Uint8 *stream, int len);
#else
void ProcessAudioPlayback(Sint16 *stream, int sampleCount);
#endif
void ProcessAudioMixing(Sint32 *dst, const Sint16 *src, int len, int volume, sbyte pan);
#endif

#if !RETRO_USE_ORIGINAL_CODE
inline void FreeMusInfo(bool Lock = true)
{
    if (Lock) LockAudioDevice();

#if RETRO_USING_SDL2
    if (streamInfo[currentStreamIndex].stream)
        SDL_FreeAudioStream(streamInfo[currentStreamIndex].stream);
    streamInfo[currentStreamIndex].stream = NULL;
#endif

    if (streamInfo[currentStreamIndex].loaded) {
        ov_clear(&streamInfo[currentStreamIndex].vorbisFile);
        streamInfo[currentStreamIndex].loaded = false;
    }

#if RETRO_USING_SDL2
    streamInfo[currentStreamIndex].stream = nullptr;
#endif

    if (Lock) UnlockAudioDevice();
}
#endif

void LoadMusic(void *userdata);
void SetMusicTrack(const char *filePath, byte trackID, bool loop, uint loopPoint);
void SwapMusicTrack(const char *filePath, byte trackID, uint loopPoint, uint ratio);
bool PlayMusic(int track, int musStartPos);
inline void StopMusic(bool setStatus)
{
    if (setStatus)
        musicStatus = MUSIC_STOPPED;
    musicPosition = 0;

#if !RETRO_USE_ORIGINAL_CODE
    FreeMusInfo(true);
#endif
}

void LoadSfx(char *filePath, byte sfxID);
void PlaySfx(int sfx, bool loop);
inline void StopSfx(int sfx)
{
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (sfxChannels[i].sfxID == sfx) {
            MEM_ZERO(sfxChannels[i]);
            sfxChannels[i].sfxID = -1;
        }
    }
}
void SetSfxAttributes(int sfx, int loopCount, sbyte pan);

void SetSfxName(const char *sfxName, int sfxID);

#if !RETRO_USE_ORIGINAL_CODE
// Helper Funcs
inline bool PlaySfxByName(const char *sfx, sbyte loopCnt)
{
    char buffer[0x40];
    int pos = 0;
    while (*sfx) {
        if (*sfx != ' ')
            buffer[pos++] = *sfx;
        sfx++;
    }
    buffer[pos] = 0;

    for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
        if (StrComp(sfxNames[s], buffer)) {
            PlaySfx(s, loopCnt);
            return true;
        }
    }
    return false;
}
inline bool StopSFXByName(const char *sfx)
{
    char buffer[0x40];
    int pos = 0;
    while (*sfx) {
        if (*sfx != ' ')
            buffer[pos++] = *sfx;
        sfx++;
    }
    buffer[pos] = 0;

    for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
        if (StrComp(sfxNames[s], buffer)) {
            StopSfx(s);
            return true;
        }
    }
    return false;
}
#endif

inline void SetMusicVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > MAX_VOLUME)
        volume = MAX_VOLUME;

    masterVolume = volume;
}

inline void SetGameVolumes(int bgmVol, int sfxVol)
{
    bgmVolume = bgmVol;
    sfxVolume = sfxVol;

    if (bgmVolume < 0)
        bgmVolume = 0;
    if (bgmVolume > MAX_VOLUME)
        bgmVolume = MAX_VOLUME;

    if (sfxVolume < 0)
        sfxVolume = 0;
    if (sfxVolume > MAX_VOLUME)
        sfxVolume = MAX_VOLUME;
}

inline bool PauseSound()
{
    if (musicStatus == MUSIC_PLAYING) {
        musicStatus = MUSIC_PAUSED;
        return true;
    }
    return false;
}

inline void ResumeSound()
{
    if (musicStatus == MUSIC_PAUSED)
        musicStatus = MUSIC_PLAYING;
}

inline void StopAllSfx()
{
#if !RETRO_USE_ORIGINAL_CODE
    LockAudioDevice();
#endif

    for (int i = 0; i < CHANNEL_COUNT; ++i) sfxChannels[i].sfxID = -1;

#if !RETRO_USE_ORIGINAL_CODE
    UnlockAudioDevice();
#endif
}
inline void ReleaseGlobalSfx()
{
    for (int i = globalSFXCount - 1; i >= 0; --i) {
        if (sfxList[i].loaded) {
            StrCopy(sfxList[i].name, "");
            StrCopy(sfxNames[i], "");
            if (sfxList[i].buffer)
                free(sfxList[i].buffer);

            sfxList[i].buffer = NULL;
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }

    globalSFXCount = 0;
}
inline void ReleaseStageSfx()
{
    for (int i = (stageSFXCount + globalSFXCount) - 1; i >= globalSFXCount; --i) {
        if (sfxList[i].loaded) {
            StrCopy(sfxList[i].name, "");
            StrCopy(sfxNames[i], "");
            if (sfxList[i].buffer)
                free(sfxList[i].buffer);

            sfxList[i].buffer = NULL;
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }

    stageSFXCount = 0;
}

inline void ReleaseAudioDevice()
{
    StopMusic(true);
    StopAllSfx();
    ReleaseStageSfx();
    ReleaseGlobalSfx();
}

#endif // !AUDIO_H