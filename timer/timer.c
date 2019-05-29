#include "hrbstd.h"

void HariMain(){
    // 描画バッファ確保
    api_init_memman();
    char *window_buf = api_malloc(150 * 50);

    // ウィンドウ生成
    int win = api_gen_window(window_buf, 150, 50, -1, "Timer");

    // タイマー初期化
    int timer = api_alloc_timer();
    api_init_timer(timer, 128);

    // メインループ
    int sec = 0;
    while(1){
        // 表示部分
        char str[40];
        sprintf(str, "%d:%d:%d", sec / 3600, sec / 60, sec % 60);
        api_boxfill_window(win, 28, 27, 115, 41, 7);
        api_putstr_window(win, 28, 27, 0, 11, str);
        api_refresh_window(win, 28, 27, 116, 42);

        // 1秒ごと
        api_set_timer(timer, 100);
        if(api_keyinput(1) != 128){
            break;
        }
        ++ sec;
    }

    api_end();
}