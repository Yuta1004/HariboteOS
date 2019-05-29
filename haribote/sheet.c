#include "bootpack.h"

// SHEET_CTL初期化
struct SHEET_CTL *sheet_ctl_init(struct MEMMAN *memman, unsigned char *vram, int width, int height){
    // 構造体初期化(メモリ確保も同時に)
    struct SHEET_CTL *sheet_ctl = (struct SHEET_CTL *) memman_alloc_4k(memman, sizeof(struct SHEET_CTL));
    if(sheet_ctl == 0){
        return 0;
    }

    // 描画マップ初期化
    sheet_ctl->g_map = (unsigned char *) memman_alloc_4k(memman, width * height);
    if(sheet_ctl->g_map == 0){
        memman_free_4k(memman, (int)sheet_ctl, sizeof(struct SHEET_CTL));
        goto err;
    }

    // 構造体設定
    sheet_ctl->vram = vram;
    sheet_ctl->width = width;
    sheet_ctl->height = height;
    sheet_ctl->top = -1;
    for(int idx = 0; idx < MAX_SHEETS; idx ++){
        sheet_ctl->sheet_data[idx].flags = 0;
        sheet_ctl->sheet_data[idx].sheet_ctl = sheet_ctl;
    }

err:
    return sheet_ctl;
}

// シート新規確保
struct SHEET *sheet_alloc(struct SHEET_CTL *sheet_ctl){
    struct SHEET *sheet;

    // 未使用のシートを見つける
    for(int idx = 0; idx < MAX_SHEETS; idx ++){
        if(sheet_ctl->sheet_data[idx].flags == 0){
            sheet = &sheet_ctl->sheet_data[idx];
            sheet->flags = SHEET_USE;
            sheet->layer_height = -1;
            sheet->task = 0;
            return sheet;
        }
    }

    // 見つからなかった
    return 0;
}

// シート初期化
void sheet_setbuf(struct SHEET *sheet, unsigned char *buf, int width, int height, int col_inv){
    sheet->buf = buf;
    sheet->buf_width = width;
    sheet->buf_height = height;
    sheet->col_inv = col_inv;
}

// シートの高さを再設定する
void sheet_updown(struct SHEET *sheet, int height){
    struct SHEET_CTL *sheet_ctl = sheet->sheet_ctl;
    int old_height = sheet->layer_height;

    // 指定の高さが低すぎ・高すぎの場合修正
    if(height > sheet_ctl->top + 1){
        height = sheet_ctl->top + 1;
    }
    if(height < -1){
        height = -1;
    }

    // 高さ設定
    sheet->layer_height = height;

    // *sheets[]の並べ替え
    // 添え字が大きいほどシートの位置が高い
    if(old_height > sheet->layer_height){    // シートが以前より低くなる
        if(height >= 0){
            for(int h_val = old_height; h_val > height; h_val --){
                sheet_ctl->sheets[h_val] = sheet_ctl->sheets[h_val-1];
                sheet_ctl->sheets[h_val]->layer_height = h_val;
            }
            sheet_ctl->sheets[height] = sheet;
            sheet_refresh_map(sheet->sheet_ctl, sheet->vram_x, sheet->vram_y,
                              sheet->vram_x+sheet->buf_width, sheet->vram_y + sheet->buf_height, height + 1);
            sheet_refresh_with_range(sheet->sheet_ctl, sheet->vram_x, sheet->vram_y,
                                     sheet->vram_x+sheet->buf_width, sheet->vram_y + sheet->buf_height, height + 1, old_height);
        }else{  // シートを非表示にする場合
            if(sheet_ctl->top > old_height){
                for(int h_val = old_height; h_val < sheet_ctl->top; h_val ++){
                    sheet_ctl->sheets[h_val] = sheet_ctl->sheets[h_val + 1];
                    sheet_ctl->sheets[h_val]->layer_height = h_val;
                }
            }
            sheet_ctl->top --;
            sheet_refresh_map(sheet->sheet_ctl, sheet->vram_x, sheet->vram_y,
                              sheet->vram_x+sheet->buf_width, sheet->vram_y + sheet->buf_height, 0);
            sheet_refresh_with_range(sheet->sheet_ctl, sheet->vram_x, sheet->vram_y,
                                     sheet->vram_x+sheet->buf_width, sheet->vram_y + sheet->buf_height, 0, old_height - 1);
        }
    }
    else if(old_height < sheet->layer_height){  // シートが以前よりも高くなる
        if(old_height >= 0){
            for(int h_val = old_height; h_val < height; h_val ++){
                sheet_ctl->sheets[h_val] = sheet_ctl->sheets[h_val + 1];
                sheet_ctl->sheets[h_val]->layer_height = h_val;
            }
        }else{  // シートが非表示から表示状態になる時
            for(int h_val = sheet_ctl->top; h_val >= height; h_val --){
                sheet_ctl->sheets[h_val + 1] = sheet_ctl->sheets[h_val];
                sheet_ctl->sheets[h_val + 1]->layer_height = h_val + 1;
            }
            sheet_ctl->top ++;
        }

        sheet_ctl->sheets[height] = sheet;
        sheet_refresh_map(sheet->sheet_ctl, sheet->vram_x, sheet->vram_y,
                              sheet->vram_x+sheet->buf_width, sheet->vram_y + sheet->buf_height, height);
        sheet_refresh_with_range(sheet->sheet_ctl, sheet->vram_x, sheet->vram_y,
                                 sheet->vram_x+sheet->buf_width, sheet->vram_y + sheet->buf_height, height, height);
    }
}

// 描画マップ更新
void sheet_refresh_map(struct SHEET_CTL *sheet_ctl, int vram_x0, int vram_y0, int vram_x1, int vram_y1, int s_height){
    unsigned char *vram = sheet_ctl->vram;

    // シート全てで…
    for(int h_val = s_height; h_val <= sheet_ctl->top; h_val ++){
        struct SHEET *sheet = sheet_ctl->sheets[h_val];
        unsigned char *buf = sheet->buf;

        // メモリ番地を引き算してシートIDを取得する
        int sheet_id = sheet - sheet_ctl->sheet_data;

        // 描画対象が画面外だった場合修正する
        if(vram_x0 < 0) vram_x0 = 0;
        if(vram_y0 < 0) vram_y0 = 0;
        if(vram_x1 > sheet_ctl->width) vram_x1 = sheet_ctl->width;
        if(vram_y1 > sheet_ctl->height) vram_y1 = sheet_ctl->height;

        // 各シートのバッファを基準とした座標を逆算する
        int buf_x0 = vram_x0 - sheet->vram_x;
        int buf_y0 = vram_y0 - sheet->vram_y;
        int buf_x1 = vram_x1 - sheet->vram_x;
        int buf_y1 = vram_y1 - sheet->vram_y;

        // シートの外に出てしまった場合は修正する
        if(buf_x0 < 0) buf_x0 = 0;
        if(buf_y0 < 0) buf_y0 = 0;
        if(buf_x1 > sheet->buf_width) buf_x1 = sheet->buf_width;
        if(buf_y1 > sheet->buf_height) buf_y1 = sheet->buf_height;

        // 指定範囲だけマップ更新
        if(sheet->col_inv == -1){   // 透明色指定なし(ちょっと早い)
            if((sheet->vram_x & 3) == 0 && (buf_x0 & 3) == 0 && (buf_x1 & 3) == 0){    // シートサイズ・座標が4の倍数なら
                // 同時に4バイトを書き込む高速版
                buf_x1 = (buf_x1 - buf_x0) / 4;
                int sid4 = sheet_id | sheet_id << 8 | sheet_id << 16 | sheet_id << 24;
                for(int y = buf_y0; y < buf_y1; ++ y){
                    int vram_x = sheet->vram_x + buf_x0;
                    int vram_y = sheet->vram_y + y;
                    int *wp = (int *) &sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x];
                    for(int x = 0; x < buf_x1; ++ x){
                       wp[x] = sid4;
                    }
                }
            }else{
                // 普通（ちょっと早い
                for(int y = buf_y0; y < buf_y1; y ++){
                    for(int x = buf_x0; x < buf_x1; x ++){
                        int vram_x = sheet->vram_x + x;
                        int vram_y = sheet->vram_y + y;

                        char color = buf[y * sheet->buf_width + x];
                        sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x] = sheet_id;
                    }
                }
            }
        }else{                      // 透明色指定あり
            for(int y = buf_y0; y < buf_y1; y ++){
                for(int x = buf_x0; x < buf_x1; x ++){
                    int vram_x = sheet->vram_x + x;
                    int vram_y = sheet->vram_y + y;

                    char color = buf[y * sheet->buf_width + x];
                    if(color != sheet->col_inv){
                        sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x] = sheet_id;
                    }
                }
            }
        }
    }
}

// シート描画(特定シートの指定範囲)
void sheet_refresh(struct SHEET *sheet, int buf_x0, int buf_y0, int buf_x1, int buf_y1){
    if(sheet->layer_height >= 0){
        int base_x = sheet->vram_x, base_y = sheet->vram_y;
        sheet_refresh_with_range(sheet->sheet_ctl, base_x + buf_x0, base_y + buf_y0, base_x + buf_x1, base_y + buf_y1,
                                 sheet->layer_height, sheet->layer_height);
    }
}

// シート描画(VRAM範囲指定)
void sheet_refresh_with_range(struct SHEET_CTL *sheet_ctl, int vram_x0, int vram_y0, int vram_x1, int vram_y1, int s_height, int e_height){
    unsigned char *vram = sheet_ctl->vram;

    // シート全てで…
    for(int h_val = s_height; h_val <= e_height; h_val ++){
        struct SHEET *sheet = sheet_ctl->sheets[h_val];
        unsigned char *buf = sheet->buf;

        // メモリ番地を引き算してシートIDを取得する
        int sheet_id = sheet - sheet_ctl->sheet_data;

        // 描画対象が画面外だった場合修正する
        if(vram_x0 < 0) vram_x0 = 0;
        if(vram_y0 < 0) vram_y0 = 0;
        if(vram_x1 > sheet_ctl->width) vram_x1 = sheet_ctl->width;
        if(vram_y1 > sheet_ctl->height) vram_y1 = sheet_ctl->height;

        // 各シートのバッファを基準とした座標を逆算する
        int buf_x0 = vram_x0 - sheet->vram_x;
        int buf_y0 = vram_y0 - sheet->vram_y;
        int buf_x1 = vram_x1 - sheet->vram_x;
        int buf_y1 = vram_y1 - sheet->vram_y;

        // シートの外に出てしまった場合は修正する
        if(buf_x0 < 0) buf_x0 = 0;
        if(buf_y0 < 0) buf_y0 = 0;
        if(buf_x1 > sheet->buf_width) buf_x1 = sheet->buf_width;
        if(buf_y1 > sheet->buf_height) buf_y1 = sheet->buf_height;

        // 指定範囲だけ再描画
        if((sheet->vram_x & 3) == 0){       // 表示座標が4の倍数なら高速化
            int sid4 = sheet_id | sheet_id << 8 | sheet_id << 16 | sheet_id << 24;
            int center_draw_size = (buf_x1 / 4) - ((buf_x0 + 3) / 4);
            for(int y = buf_y0; y < buf_y1; ++ y){
                // |前の端数部分|4の倍数部分|残りの部分| …という感じで分けて描画を行う
                // 前の端数部分
                int vram_y = sheet->vram_y + y;
                int x = buf_x0;
                for( ; x < buf_x1 && (x & 3) != 0; ++ x){
                    int vram_x = sheet->vram_x + x;
                    if(sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x] == sheet_id){
                        vram[vram_y * sheet_ctl->width + vram_x] = buf[y * sheet->buf_width + x];
                    }
                }

                // 4の倍数部分
                int vram_x = sheet->vram_x + x;
                int *map_p = (int *) &sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x];
                int *vram_p = (int *) &vram[vram_y * sheet_ctl->width + vram_x];
                int *buf_p = (int *) &sheet->buf[y * sheet->buf_width + x];
                for(int idx = 0; idx < center_draw_size; ++ idx){
                    if(map_p[idx] == sid4){     // 連続する4つが同じSHEET_IDだった
                        vram_p[idx] = buf_p[idx];
                    }else{                      // 一度に更新できないのでそれぞれ確認する
                        int x2 = x + idx * 4;
                        vram_x = sheet->vram_x + x2;
                        for(int idx_part = 0; idx_part < 4; ++ idx_part){
                            if(sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x + idx_part] == sheet_id){
                                vram[vram_y * sheet_ctl->width + vram_x + idx_part] = buf[y * sheet->buf_width + x2 + idx_part];
                            }
                        }
                    }
                }

                // 残りの部分
                for(x += center_draw_size * 4; x < buf_x1; ++ x){
                    vram_x = sheet->vram_x + x;
                    if(sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x] == sheet_id){
                        vram[vram_y * sheet_ctl->width + vram_x] = buf[y * sheet->buf_width + x];
                    }
                }
            }
        }else{                              // 普通の再描画(ちょっと遅い)
            for(int y = buf_y0; y < buf_y1; y ++){
                for(int x = buf_x0; x < buf_x1; x ++){
                    int vram_x = sheet->vram_x + x;
                    int vram_y = sheet->vram_y + y;

                    char color = buf[y * sheet->buf_width + x];
                    if(sheet_ctl->g_map[vram_y * sheet_ctl->width + vram_x] == sheet_id){
                        vram[vram_y * sheet_ctl->width + vram_x] = color;
                    }
                }
            }
        }
    }
}

// シート移動
void sheet_slide(struct SHEET *sheet, int vram_x, int vram_y){
    int old_vram_x = sheet->vram_x, old_vram_y = sheet->vram_y;
    int width = sheet->buf_width, height = sheet->buf_height;
    sheet->vram_x = vram_x;
    sheet->vram_y = vram_y;
    if(sheet->layer_height >= 0){
        sheet_refresh_map(sheet->sheet_ctl, old_vram_x, old_vram_y, old_vram_x + width, old_vram_y + height, 0);
        sheet_refresh_map(sheet->sheet_ctl, vram_x, vram_y, vram_x + width, vram_y + height, sheet->layer_height);
        sheet_refresh_with_range(sheet->sheet_ctl, old_vram_x, old_vram_y, old_vram_x + width, old_vram_y + height,
                                 0, sheet->layer_height - 1);
        sheet_refresh_with_range(sheet->sheet_ctl, vram_x, vram_y, vram_x + width, vram_y + height,
                                 sheet->layer_height, sheet->layer_height);
    }
}

// シート解放
void sheet_free(struct SHEET *sheet){
    if(sheet->layer_height >= 0){
        sheet_updown(sheet, -1);
    }
    sheet->flags = 0;
}
