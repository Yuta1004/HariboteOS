#include "hrbstd.h"

void HariMain(){
    // 描画バッファ確保
    char window_buf[150 * 70];

    // ウィンドウ生成
    int win = api_gen_window(window_buf, 150, 70, 0, "notrec");

    // 透明の四角形を描画する
    api_boxfill_window(win, 0, 54, 34, 69, 0);
    api_boxfill_window(win, 115, 50, 149, 69, 0);
    api_boxfill_window(win, 50, 30, 99, 49, 0);
    api_refresh_window(win, 0, 0, 150, 70);

    // キー入力で終了
    api_keyinput(1);
    api_end();
}
