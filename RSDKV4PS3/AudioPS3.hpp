#ifndef AUDIO_PS3_H
#define AUDIO_PS3_H

#include <stdint.h>
#include <sys/synchronization.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int16_t audio_sample_t;

bool InitPS3Audio(uint32_t samplerate, uint32_t buffersize);
void WritePS3Audio(const audio_sample_t* buffer, uint32_t samples);
uint32_t GetPS3AudioFreeSamples();
void ExitPS3Audio();

#ifdef __cplusplus
}
#endif

#endif
