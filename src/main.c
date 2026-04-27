#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h> // memalign用

// 関数のプロトタイプ宣言
void my_print(const char *msg, ...);

PrintConsole topScreenConsole;
static u8 cursor_y = 0;

int main(void)
{
    // 初期化
    gfxInitDefault();
    consoleInit(GFX_TOP, &topScreenConsole);

    // 画面のクリア
    consoleClear();

    Result res = 0;
    Handle fileHandle = 0;
    FS_Archive archive = 0;
    bool archiveOpen = false;
    bool fileOpen = false;

    // 3DSのシステムアーカイブ（Account SaveDataなど）へのアクセス
    // 0x00010032 は NNID 関連のシステムセーブデータ ID
    u32 archive_path_data[] = {0, 0x00010032};
    FS_Path save_archive_path = { PATH_BINARY, 8, archive_path_data };

    // アーカイブのオープン
    res = FSUSER_OpenArchive(&archive, ARCHIVE_SYSTEM_SAVEDATA, save_archive_path);
    if(R_FAILED(res)){
        my_print("error OpenArchive: %08lX", res);
        goto exit;
    }
    archiveOpen = true;

    // ファイルのオープン
    FS_Path file_path = fsMakePath(PATH_ASCII, "/1/account");
    res = FSUSER_OpenFile(&fileHandle, archive, file_path, FS_OPEN_READ, 0);
    if(R_FAILED(res)){
        my_print("error OpenFile: %08lX", res);
        goto exit;
    }
    fileOpen = true;

    // データの読み込み
    u8 account_file[88];
    u32 bytesRead = 0;
    res = FSFILE_Read(fileHandle, &bytesRead, 0x0, account_file, 88);
    if(R_FAILED(res)){
        my_print("error ReadFile: %08lX", res);
        goto exit;
    }

    // ヘッダーチェック (最初の8バイト)
    u64 header;
    memcpy(&header, &account_file[0], 8);
    
    // リトルエンディアンでの比較
    if(header != 0x2010102143415046ULL) {
        my_print("no match header: %016llX", header);
    } else {
        my_print("Header OK!");
    }

    // HMAC部分 (66～84バイト目付近) の抽出
    u8 hmac_data[20]; 
    int hmac_len = 18; 
    memcpy(hmac_data, &account_file[66], hmac_len);
    
    // 文字列バッファを用意
    char hmac_str[hmac_len * 2 + 1];
    for(int i = 0; i < hmac_len; i++) {
        sprintf(&hmac_str[i * 2], "%02X", hmac_data[i]);
    }
    hmac_str[hmac_len * 2] = '\0';

    my_print("HMAC: %s", hmac_str);

    // --- Socket通信セクション (必要に応じて実装) ---
    /*
    static u32* soc_buffer = NULL;
    soc_buffer = (u32*)memalign(0x1000, 0x100000);
    if(soc_buffer && R_SUCCEEDED(socInit(soc_buffer, 0x100000))) {
        my_print("SOC initialized.");
        // ここに接続・送信処理を記述
        socExit();
    }
    if(soc_buffer) free(soc_buffer);
    */

exit:
    // リソースの解放
    if(fileOpen) FSFILE_Close(fileHandle);
    if(archiveOpen) FSUSER_CloseArchive(archive);

    my_print("Press any key to exit");

    // メインループ
    while (aptMainLoop()){
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_ANY) break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}

// カスタムプリント関数
void my_print(const char *msg, ...){
    va_list args;
    va_start(args, msg);
    // 画面外にでないように制御が必要な場合は、ここで cursor_y の上限をチェックしてください
    printf("\x1b[%u;1H", ++cursor_y);
    vprintf(msg, args);
    va_end(args);
}