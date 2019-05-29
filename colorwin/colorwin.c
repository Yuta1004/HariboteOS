#include "hrbstd.h"

unsigned char rgb2pal(int r, int g, int b, int x, int y);

void HariMain(){
    // 描画バッファ確保
    api_init_memman();
    char *window_buf = api_malloc(144 * 164);

    // ウィンドウ生成
    int win = api_gen_window(window_buf, 144, 164, -1, "Colorful");

    // 綺麗なウィンドウを作る
    for(int y = 0; y < 128; ++ y){
        for(int x = 0; x < 128; ++ x){
            window_buf[(x + 8) + (y + 28) * 144] = rgb2pal(x * 2, y * 2, 0, x, y);
        }
    }
    api_refresh_window(win, 8, 28, 136, 156);

    api_keyinput(1);
    api_end();
}

unsigned char rgb2pal(int r, int g, int b, int x, int y){
    static int color_set_table[4] = {3, 1, 0, 2};
    x &= 1;     // 偶奇
    y &= 1;
    int color_info = color_set_table[x + y * 2];

    r = (r * 21) / 256;
    g = (g * 21) / 256;
    b = (b * 21) / 256;
    r = (r + color_info) / 4;
    g = (g + color_info) / 4;
    b = (b + color_info) / 4;

    return 16 + r + g * 6 + b * 36;
}
