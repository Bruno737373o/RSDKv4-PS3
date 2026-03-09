#ifndef PS3_SAVEDATA_H
#define PS3_SAVEDATA_H

#if RETRO_PLATFORM == RETRO_PS3

#include <sysutil/sysutil_savedata.h>
#include "Ini.hpp"

#define PS3_SAVE_DIR "/dev_hdd0/tmp/"

void InitPS3SaveData();
void FetchFromSaveData();
void CommitToSaveData();

extern bool ps3_save_op_done;

#endif

#endif
