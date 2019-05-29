#include "bootpack.h"

// パレット初期化
void init_palette(void){
    // staticをつける事でDB命令相当となる
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,   // 0: 黒
        0xff, 0x00, 0x00,   // 1: 明るい赤
        0x00, 0xff, 0x00,   // 2: 明るい緑
        0xff, 0xff, 0x00,   // 3: 明るい黄色
        0x00, 0x00, 0xff,   // 4: 明るい青
        0xff, 0x00, 0xff,   // 5: 明るい紫
        0x00, 0xff, 0xff,   // 6: 明るい水色
        0xff, 0xff, 0xff,   // 7: 白
        0xc6, 0xc6, 0xc6,   // 8: 明るい灰色
        0x84, 0x00, 0x00,   // 9: 暗い赤
        0x00, 0x84, 0x00,   // 10: 暗い緑
        0x84, 0x84, 0x00,   // 11: 暗い黄色
        0x00, 0x00, 0x84,   // 12: 暗い青
        0x84, 0x00, 0x84,   // 13: 暗い紫
        0x00, 0x84, 0x84,   // 14: 暗い水色
        0x84, 0x84, 0x84    // 15: 暗い灰色
    };
    set_palette(0, 15, table_rgb);

    // +216色
    unsigned char table_rgb_alpha[216 * 3];
    for(int b = 0; b < 6; ++ b){
        for(int g = 0; g < 6; ++ g){
            for(int r = 0; r < 6; ++ r){
                table_rgb_alpha[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
                table_rgb_alpha[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
                table_rgb_alpha[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
            }
        }
    }
    set_palette(16, 231, table_rgb_alpha);

    return;
}

// パレット設定
void set_palette(int start, int end, unsigned char *rgb){
    int eflags = io_load_eflags();  // 割り込みフラグの状態を記録する
    io_cli();                       // 許可フラグを0にして割り込み禁止にする

    // パレットに情報を書き込んでいる
    io_out8(0x03c8, start);
    for(int i = 0; i <= end; i++){
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }

    io_store_eflags(eflags);        // 割り込みフラグを元に戻す
    return;
}

// 画面初期化
void init_screen(unsigned char *vram, int width, int height){
    // 背景 + タスクバーの後ろ
    boxfill8(vram, width, COL8_008484,          0,           0,     width-1,   height-29);
    boxfill8(vram, width, COL8_C6C6C6,          0,   height-28,     width-1,   height-28);
    boxfill8(vram, width, COL8_FFFFFF,          0,   height-27,     width-1,   height-27);
    boxfill8(vram, width, COL8_C6C6C6,          0,   height-26,     width-1,    height-1);

    // 左下のボタン
    boxfill8(vram, width, COL8_FFFFFF,          3,   height-24,          59,   height-24);
    boxfill8(vram, width, COL8_FFFFFF,          2,   height-24,           2,    height-4);
    boxfill8(vram, width, COL8_848484,          3,    height-4,          59,    height-4);
    boxfill8(vram, width, COL8_848484,         59,   height-23,          59,    height-5);
    boxfill8(vram, width, COL8_000000,          2,    height-3,          59,    height-3);
    boxfill8(vram, width, COL8_000000,         60,   height-24,          60,    height-3);

    // 右下のボタン
    boxfill8(vram, width, COL8_848484,   width-47,   height-24,     width-4,    height-24);
    boxfill8(vram, width, COL8_848484,   width-47,   height-23,    width-47,     height-4);
    boxfill8(vram, width, COL8_FFFFFF,   width-47,    height-3,     width-4,     height-3);
    boxfill8(vram, width, COL8_FFFFFF,    width-3,   height-24,     width-3,     height-3);
}

// 背景塗りつぶし & 文字列描画
void putstr8_ref(struct SHEET *sheet, int x, int y, int color, int bc_color, char *str, int len){
    struct TASK *task = task_now();
    boxfill8(sheet->buf, sheet->buf_width, bc_color, x, y, x + len * 8 - 1, y + 15);
    putstr8(sheet->buf, sheet->buf_width, x, y, color, str);

    if(task->langmode != 0 && task->langbyte1 != 0){
        sheet_refresh(sheet, x - 8, y, x + len * 8, y + 16);
    }else{
        sheet_refresh(sheet, x, y, x + len * 8, y + 16);
    }
}

// 四角形を描画する (x0, y0) -> (x1, y1)
void boxfill8(unsigned char *vram, int width, unsigned char color, int x0, int y0, int x1, int y1){
    for(int y = y0; y <= y1; y++){
        for(int x = x0; x <= x1; x++){
            vram[y * width + x] = color;   // 対応座標のvramの番地を求めて色を書き込んでいる
        }
    }
}

// 1文字ずつ描画する
void putfont8(unsigned char *vram, int width, int x, int y, unsigned char color, char *font){
    char *p, line_data;

    // 各列上位ビットから見ていって1だったら対応座標に点を打つ
    for(int line = 0; line < 16; line++){
        p =  vram + (y + line) * width + x;
        line_data = font[line];

        for(int bit = 0; bit < 8; bit++){
            if((line_data & (0x80 >> bit)) != 0){
                p[bit] = color;
            }
        }
    }
}

// 文字列描画
void putstr8(unsigned char *vram, int width, int x, int y, unsigned char color, unsigned char *str){
    extern char hankaku[4096];
    struct TASK *task = task_now();
    char *japanese = (char *) *((int *) 0x0fe8);

    // 内臓フォントモード
    if(task->langmode == 0){
        for(; *str != 0x00; ++ str){
            putfont8(vram, width, x, y, color, hankaku + *str * 16);
            x += 8;
        }
        return;
    }

    // Shift-JIS
    if(task->langmode == 1){
        for(; *str != 0x00; str++){
            if(task->langbyte1 == 0){   // 1バイト目
                if((0x81 <= *str && *str <= 0x9f) || (0xe0 <= *str && *str <= 0xfc)){
                    task->langbyte1 = *str;
                }else{
                    putfont8(vram, width, x, y, color, japanese + *str * 16);
                }
            }else{                      // 2バイト目
                // 区
                int k;
                if(0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f){
                    k = (task->langbyte1 - 0x81) * 2;
                }else{
                    k = (task->langbyte1 - 0xe0) * 2 + 62;
                }

                // 点
                int t;
                if(0x40 <= *str && *str <= 0x7e){
                    t = *str - 0x40;
                }else if(0x80 <= *str && *str <= 0x9e){
                    t = *str - 0x80 + 63;
                }else{
                    t = *str - 0x9f;
                    ++ k;
                }

                // 出力
                char *font = japanese + 256 * 16 + (k * 94 + t) * 32;
                putfont8(vram, width, x - 8, y, color, font);
                putfont8(vram, width, x, y, color, font + 16);
                task->langbyte1 = 0;
            }
            x += 8;
        }
        return;
    }

    // EUC
    if(task->langmode == 2){
        for(; *str != 0x00; ++ str){
            if(task->langbyte1 == 0){       // 1バイト目
                if(0x81 <= *str && *str <= 0xfe){
                    task->langbyte1 = *str;
                }else{
                    putfont8(vram, width, x, y, color, japanese + *str * 16);
                }
            }else{                          // 2バイト目
                int k = task->langbyte1 - 0xa1;
                int t = *str - 0xa1;
                char *font = japanese + 256 * 16 + (k * 94 + t) * 32;
                putfont8(vram, width, x - 8, y, color, font);
                putfont8(vram, width, x, y, color, font + 16);
                task->langbyte1 = 0;
            }
        }
    }
}

// マウスカーソル描画準備
void init_mouse_cursor8(char *mouse, char bc_color){
    static char cursor[16][16] = {
        "**************..",   // 1
        "*ooooooooooo*...",   // 2
        "*oooooooooo*....",   // 3
        "*ooooooooo*.....",   // 4
        "*oooooooo*......",   // 5
        "*ooooooo*.......",   // 6
        "*ooooooo*.......",   // 7
        "*oooooooo*......",   // 8
        "*oooo**ooo*.....",   // 9
        "*ooo*..*ooo*....",   // 10
        "*oo*....*ooo*...",   // 11
        "*o*......*ooo*..",   // 12
        "**........*ooo*.",   // 13
        "*..........*ooo*",   // 14
        ".. .........*oo*",   // 15
        ".............***",   // 12
  };

  for (int y = 0; y < 16; y ++){
      for (int x = 0; x < 16; x ++){
          if (cursor[y][x] == '*'){
              mouse[y * 16 + x] = COL8_000000;
          }else if (cursor[y][x] == 'o'){
              mouse[y * 16 + x] = COL8_FFFFFF;
          }else{
              mouse[y * 16 + x] = bc_color;
          }
      }
  }
}

void putblock8_8(unsigned char *vram, int vwidth, int pwidth, int pheight, int px0, int py0, char *buf, int bwidth){
  for (int y = 0; y < pheight; y ++){
      for (int x = 0; x < pwidth; x ++){
          vram[(py0 + y) * vwidth + (px0 + x)] = buf[y * bwidth + x];
      }
  }
}

void make_textbox8(struct SHEET *sheet, int x0, int y0, int width, int height, int color){
    int x1 = x0 + width, y1 = y0 + height;
    boxfill8(sheet->buf, sheet->buf_width, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sheet->buf, sheet->buf_width, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sheet->buf, sheet->buf_width, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sheet->buf, sheet->buf_width, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sheet->buf, sheet->buf_width, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sheet->buf, sheet->buf_width, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sheet->buf, sheet->buf_width, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sheet->buf, sheet->buf_width, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sheet->buf, sheet->buf_width, color,       x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    sheet_refresh(sheet, x0 - 5, y0 - 5, x1 + 5, y1 + 5);
}

void change_window_title(struct SHEET *sheet, char action){
    // 色決定
    char title_color_new, title_b_color_new, title_color_old, title_b_color_old;
    if(action == 1){
        title_color_new = COL8_FFFFFF;
        title_b_color_new = COL8_000084;
        title_color_old = COL8_C6C6C6;
        title_b_color_old = COL8_848484;
    }else{
        title_color_new = COL8_C6C6C6;
        title_b_color_new = COL8_848484;
        title_color_old = COL8_FFFFFF;
        title_b_color_old = COL8_000084;
    }

    // 更新
    for(int y = 3; y < 20; ++ y){
        for(int x = 3; x <= sheet->buf_width - 4; ++ x){
            char color = sheet->buf[sheet->buf_width * y + x];
            if(color == title_color_old && x <= sheet->buf_width - 22){
                color = title_color_new;
            }else if(color == title_b_color_old){
                color = title_b_color_new;
            }
            sheet->buf[sheet->buf_width * y + x] = color;
        }
    }
    sheet_refresh(sheet, 3, 3, sheet->buf_width, 21);
}

void drawline(struct SHEET *sheet, int x0, int y0, int x1, int y1, int color){
    int dx = x1 - x0;
    int dy = y1 - y0;
    int x = x0 << 10;
    int y = y0 << 10;
    int len = 0;

    if(dx < 0) dx = -dx;
    if(dy < 0) dy = -dy;

    // 変化量設定
    // (2 * (bool) - 1) => boolがtrueなら1, falseなら-1を返す
    if(dy <= dx){
        len = dx + 1;
        dx = 1024 * (2 * (x0 < x1) - 1);
        dy = (y1 - y0 + (2 * (y0 <= y1) - 1) << 10) / len;
    }else{
        len = dy + 1;
        dy = 1024 * (2 * (y0 < y1) - 1);
        dx = (x1 - x0 + (2 * (x0 <= x1) - 1) << 10) / len;
    }

    for(int cnt = 0; cnt < len; ++ cnt){
        sheet->buf[sheet->buf_width * (y >> 10) + (x >> 10)] = color;
        x += dx;
        y += dy;
    }
}