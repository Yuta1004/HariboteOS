#include "bootpack.h"

void make_window(unsigned char *buf, int width, int height, char *title, char is_active){
    // ウィンドウ本体
    boxfill8(buf, width, COL8_C6C6C6,           0,           0, width - 1,  0         );
    boxfill8(buf, width, COL8_FFFFFF,           1,           1, width - 1,  1         );
    boxfill8(buf, width, COL8_C6C6C6,           0,           0, 0,          height - 1);
    boxfill8(buf, width, COL8_FFFFFF,           1,           1, 1,          height - 2);
    boxfill8(buf, width, COL8_848484,   width - 2,           1, width - 2,  height - 2);
    boxfill8(buf, width, COL8_000000,   width - 1,           0, width - 1,  height - 1);
    boxfill8(buf, width, COL8_C6C6C6,           2,           2, width - 3,  height - 3);
    boxfill8(buf, width, COL8_848484,           1,  height - 2, width - 2,  height - 2);
    boxfill8(buf, width, COL8_000000,           0,  height - 1, width - 1,  height - 1);
    make_window_title(buf, width, title, is_active);
}

void make_window_title(unsigned char *buf, int width, char *title, char is_active){
    // ウィンドウがアクティブ状態なら
    char title_color, title_b_color;
    if(is_active == 1){
        title_color = COL8_FFFFFF;
        title_b_color = COL8_000084;
    }else{
        title_color = COL8_C6C6C6;
        title_b_color = COL8_848484;
    }

    // 閉じるボタン
    static char close_button[14][16] ={
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "QQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };

    // タイトル文字列描画
    boxfill8(buf, width, title_b_color, 3, 3, width - 4, 20);
    putstr8(buf, width, 24, 4, title_color, title);

    // 閉じるボタン描画
    for(int y = 0; y < 14; ++ y){
        for(int x = 0; x < 16; ++ x){
            char c = close_button[y][x];
            if(c == '@'){
                c = COL8_000000;
            }else if(c == '$'){
                c = COL8_848484;
            }else if(c == 'Q'){
                c = COL8_C6C6C6;
            }else{
                c = COL8_FFFFFF;
            }

            buf[(5 + y) * width + (width - 21 + x)] = c;
        }
    }
}

// 指定ウィンドウを入力モードにする
void key_window_on(struct SHEET *key_to_window){
    change_window_title(key_to_window, 1);
    if((key_to_window->flags & 0x20) != 0){
        fifo32_put(&key_to_window->task->fifo, 3);
    }
}

// 指定ウィンドウを入力モード解除する
void key_window_off(struct SHEET *key_to_window){
    change_window_title(key_to_window, 0);
    if((key_to_window->flags & 0x20) != 0){
        fifo32_put(&key_to_window->task->fifo, 2);
    }
}
