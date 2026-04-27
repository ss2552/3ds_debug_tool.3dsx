#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

void my_print(const char *msg, ...);

PrintConsole topScreenConsole;
static u8 cursor_y = 0;

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, &topScreenConsole);
    consoleClear();

    Result res = 0;
    Handle fileHandle = 0;
    FS_Archive archive = 0;
    bool archiveOpen = false;
    bool fileOpen = false;
    
    // エラー回避のため、バッファを固定長で先に宣言しておく（18*2+1=37なので64あれば十分）
    char hmac_str[64]; 
    memset(hmac_str, 0, sizeof(hmac_str));

    u32 archive_path_data[] = {0, 0x00010032};
    FS_Path save_archive_path = { PATH_BINARY, 8, archive_path_data };

    res = FSUSER_OpenArchive(&archive, ARCHIVE_SYSTEM_SAVEDATA, save_archive_path);
    if(R_FAILED(res)){
        my_print("error OpenArchive: %08lX", res);
        goto exit;
    }
    archiveOpen = true;

    FS_Path file_path = fsMakePath(PATH_ASCII, "/1/account");
    res = FSUSER_OpenFile(&fileHandle, archive, file_path, FS_OPEN_READ, 0);
    if(R_FAILED(res)){
        my_print("error OpenFile: %08lX", res);
        goto exit;
    }
    fileOpen = true;

    u8 account_file[88];
    u32 bytesRead = 0;
    res = FSFILE_Read(fileHandle, &bytesRead, 0x0, account_file, 88);
    if(R_FAILED(res)){
        my_print("error ReadFile: %08lX", res);
        goto exit;
    }

    u64 header;
    memcpy(&header, &account_file[0], 8);
    
    if(header != 0x2010102143415046ULL) {
        my_print("no match header: %016llX", header);
    } else {
        my_print("Header OK!");
    }

    u8 hmac_data[20]; 
    int hmac_len = 18; 
    memcpy(hmac_data, &account_file[66], hmac_len);
    
    for(int i = 0; i < hmac_len; i++) {
        sprintf(&hmac_str[i * 2], "%02X", hmac_data[i]);
    }

    my_print("HMAC: %s", hmac_str);

exit:
    if(fileOpen) FSFILE_Close(fileHandle);
    if(archiveOpen) FSUSER_CloseArchive(archive);

    my_print("Press any key to exit");

    while (aptMainLoop()){
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        // KEY_ANY の代わりに kDown が 0 でないか（何か押されたか）をチェック
        if (kDown != 0) break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}

void my_print(const char *msg, ...){
    va_list args;
    va_start(args, msg);
    printf("\x1b[%u;1H", ++cursor_y);
    vprintf(msg, args);
    va_end(args);
}