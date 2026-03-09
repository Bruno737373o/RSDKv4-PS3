#include "RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_PS3

#include "PS3SaveData.hpp"
#include <sysutil/sysutil_savedata.h>
#include <sys/timer.h>
#include <stdlib.h>
#include <string.h>

// Path in /dev_hdd0/tmp as requested by user
#define PS3_TEMP_DIR "/dev_hdd0/tmp/"

// Buffers for SaveData Utility, 128-byte aligned
static void* ps3_setbuf_mem = NULL;
bool ps3_save_op_done = false;
static int ps3_save_op_mode = 0; // 0: Load, 1: Save

struct SaveFileIO {
    const char* name;
    void* buffer;
    size_t size;
};

static SaveFileIO ps3_files[4];
static int ps3_file_count = 4;
static int ps3_file_step = 0;

static void* ps3_icon_data = NULL;
static size_t ps3_icon_size = 0;

void ps3_savedata_stat_cb(CellSaveDataCBResult *cbResult, CellSaveDataStatGet *get, CellSaveDataStatSet *set) {
    cbResult->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
    
    static CellSaveDataSystemFileParam sfo_param;
    memset(&sfo_param, 0, sizeof(CellSaveDataSystemFileParam));
#if defined(SONIC_1)
    strncpy(sfo_param.title, "Sonic the Hedgehog", CELL_SAVEDATA_SYSP_TITLE_SIZE);
#elif defined(SONIC_2)
    strncpy(sfo_param.title, "Sonic the Hedgehog 2", CELL_SAVEDATA_SYSP_TITLE_SIZE);
#else
    strncpy(sfo_param.title, "RSDKV4 PS3", CELL_SAVEDATA_SYSP_TITLE_SIZE);
#endif
    strncpy(sfo_param.subTitle, "Save Data", CELL_SAVEDATA_SYSP_SUBTITLE_SIZE);
    strncpy(sfo_param.detail, "Game Progress and Settings", CELL_SAVEDATA_SYSP_DETAIL_SIZE);
    
    set->setParam = &sfo_param;
    set->reCreateMode = CELL_SAVEDATA_RECREATE_NO;
}

void ps3_savedata_file_cb(CellSaveDataCBResult *cbResult, CellSaveDataFileGet *get, CellSaveDataFileSet *set) {
    (void)get;
    memset(set, 0, sizeof(CellSaveDataFileSet));

    if (ps3_save_op_mode == 1) { // SAVE
        if (ps3_file_step == 0) {
            if (ps3_icon_data && ps3_icon_size > 0) {
                cbResult->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
                set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
                set->fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON0;
                set->fileBuf = ps3_icon_data;
                set->fileBufSize = (ps3_icon_size + 127) & ~127;
                set->fileSize = ps3_icon_size;
                ps3_file_step = 1;
                return;
            }
            ps3_file_step = 1;
        }

        int idx = ps3_file_step - 1;
        if (idx < ps3_file_count) {
            cbResult->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
            set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
            set->fileType = CELL_SAVEDATA_FILETYPE_NORMALFILE;
            set->fileName = (char*)ps3_files[idx].name;
            set->fileBuf = ps3_files[idx].buffer;
            set->fileBufSize = (ps3_files[idx].size + 127) & ~127;
            set->fileSize = ps3_files[idx].size;
            ps3_file_step++;
            return;
        }
    } else { // LOAD
        if (ps3_file_step < ps3_file_count) {
            cbResult->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
            set->fileOperation = CELL_SAVEDATA_FILEOP_READ;
            set->fileType = CELL_SAVEDATA_FILETYPE_NORMALFILE;
            set->fileName = (char*)ps3_files[ps3_file_step].name;
            set->fileBuf = ps3_files[ps3_file_step].buffer;
            set->fileBufSize = (ps3_files[ps3_file_step].size + 127) & ~127;
            set->fileSize = ps3_files[ps3_file_step].size;
            ps3_file_step++;
            return;
        }
    }

    cbResult->result = CELL_SAVEDATA_CBRESULT_OK_LAST;
    ps3_save_op_done = true;
}

void InitPS3SaveData() {
    static bool initialized = false;
    if (initialized) return;

    if (!ps3_setbuf_mem) ps3_setbuf_mem = memalign(128, 1024 * 1024);
    
    ps3_files[0].name = "SData.bin";
    ps3_files[0].size = SAVEDATA_SIZE * sizeof(int);
    ps3_files[0].buffer = memalign(128, ps3_files[0].size);

    ps3_files[1].name = "SGame.bin";
    ps3_files[1].size = SAVEDATA_SIZE * sizeof(int);
    ps3_files[1].buffer = memalign(128, ps3_files[1].size);

    ps3_files[2].name = "UData.bin";
    ps3_files[2].size = (ACHIEVEMENT_COUNT + LEADERBOARD_COUNT) * sizeof(int);
    ps3_files[2].buffer = memalign(128, ps3_files[2].size);

    ps3_files[3].name = "settings.ini";
    ps3_files[3].size = 4096;
    ps3_files[3].buffer = memalign(128, ps3_files[3].size);

    if (!ps3_icon_data) {
        char iconPath[0x100];
        sprintf(iconPath, "%sICON0.PNG", BASE_PATH);
        FILE *f = fopen(iconPath, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            ps3_icon_size = ftell(f);
            fseek(f, 0, SEEK_SET);
            ps3_icon_data = memalign(128, (ps3_icon_size + 127) & ~127);
            fread(ps3_icon_data, 1, ps3_icon_size, f);
            fclose(f);
        }
    }
    initialized = true;
}

static void WaitPS3SaveData() {
    while (!ps3_save_op_done) {
        cellSysutilCheckCallback();
        sys_timer_usleep(1000);
    }
}

void CommitToSaveData() {
    InitPS3SaveData();
    ps3_save_op_mode = 1;
    ps3_save_op_done = false;
    ps3_file_step = 0;

    for (int i = 0; i < ps3_file_count; i++) {
        char path[128];
        sprintf(path, "%s%s", PS3_TEMP_DIR, ps3_files[i].name);
        memset(ps3_files[i].buffer, 0, ps3_files[i].size);
        FILE* f = fopen(path, "rb");
        if (f) {
            fread(ps3_files[i].buffer, 1, ps3_files[i].size, f);
            fclose(f);
        }
    }

    CellSaveDataSetBuf setBuf;
    memset(&setBuf, 0, sizeof(setBuf));
    setBuf.bufSize = 1024 * 1024;
    setBuf.buf = ps3_setbuf_mem;

    cellSaveDataAutoSave2(CELL_SAVEDATA_VERSION_420, PS3_SAVEDATA_DIRNAME, CELL_SAVEDATA_ERRDIALOG_ALWAYS, &setBuf, ps3_savedata_stat_cb, ps3_savedata_file_cb, SYS_MEMORY_CONTAINER_ID_INVALID, NULL);
    WaitPS3SaveData();
}

void FetchFromSaveData() {
    InitPS3SaveData();
    ps3_save_op_mode = 0;
    ps3_save_op_done = false;
    ps3_file_step = 0;

    for (int i = 0; i < ps3_file_count; i++) {
        memset(ps3_files[i].buffer, 0, ps3_files[i].size);
    }

    CellSaveDataSetBuf setBuf;
    memset(&setBuf, 0, sizeof(setBuf));
    setBuf.bufSize = 1024 * 1024;
    setBuf.buf = ps3_setbuf_mem;

    int ret = cellSaveDataAutoLoad2(CELL_SAVEDATA_VERSION_420, PS3_SAVEDATA_DIRNAME, CELL_SAVEDATA_ERRDIALOG_ALWAYS, &setBuf, ps3_savedata_stat_cb, ps3_savedata_file_cb, SYS_MEMORY_CONTAINER_ID_INVALID, NULL);
    
    if (ret == CELL_SAVEDATA_RET_OK) {
        WaitPS3SaveData();
        for (int i = 0; i < ps3_file_count; i++) {
            char path[128];
            sprintf(path, "%s%s", PS3_TEMP_DIR, ps3_files[i].name);
            FILE* f = fopen(path, "wb");
            if (f) {
                fwrite(ps3_files[i].buffer, 1, ps3_files[i].size, f);
                fclose(f);
            }
        }
    }
}
#endif
