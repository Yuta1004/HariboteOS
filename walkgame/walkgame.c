#include "hrbstd.h"

void HariMain(void){
    // 描画バッファ確保
    api_init_memman();
    char *window_buf = api_malloc(250 * 200);

    // ウィンドウ生成
    int win = api_gen_window(window_buf, 250, 200, -1, "Move @");
    api_boxfill_window(win + 1, 6, 26, 243, 193, 0);
    api_refresh_window(win, 6, 26, 244, 194);

    // ゲーム
    int x = 125, y = 100;
    int dx[] = {0, -3, 3, 0};
    int dy[] = {3, 0, 0, -3};
    char *messages[] = {"down", "left", "right", "up"};

    while(1){
        // キー入力
        char key_data = api_keyinput(1);
        int idx = (key_data - '2') / 2;

        // 終了
        if(key_data == 0x0a) break;

        // 出力 & 描画
        if(0 <= idx && idx <= 3){
            api_boxfill_window(win, x, y, x+8, y+16, 0);
            x += dx[idx];
            y += dy[idx];
            api_putstr_window(win, x, y, 10, 1, "@");
            api_refresh_window(win, x-3, y-3, x+11, y+19);

            api_putstr(messages[idx]);
            api_putchar('\n');
        }
    }

    api_end();
}
