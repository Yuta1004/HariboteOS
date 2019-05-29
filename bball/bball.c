#include "hrbstd.h"

void HariMain(){
    // ウィンドウ描画用バッファ
    api_init_memman();
    char *buf = api_malloc(216 * 237);

    // ボール描画用
    struct POINT{
        int x, y;
    };
    static struct POINT table[16] = {
        {204, 129}, {195, 90}, {172, 58}, {137, 38}, {98, 34},
        {61, 46}, {31, 73}, {15, 110}, {15, 148}, {31, 185},
        {61, 212}, {98, 224}, {137, 220}, {172, 200}, {195, 168},
        {204, 129}
    };

    // ウィンドウ生成
    int win = api_gen_window(buf, 216, 237, -1, "bball");
    api_boxfill_window(win, 8, 29, 207, 228, 0);

    // 描画
    for(int i = 0; i <= 14; ++ i){
        for(int j = i + 1; j <= 15; ++ j){
            int dist = j - i;
            if(dist >= 8){
                dist = 15 - dist;
            }
            if(dist != 0){
                api_drawline_window(win, table[i].x, table[i].y, table[j].x, table[j].y, 8 - dist);
            }
        }
    }
    api_refresh_window(win, 0, 0, 216, 237);

    // キー入力があるとアプリ終了
    api_keyinput(1);
    api_end();
}