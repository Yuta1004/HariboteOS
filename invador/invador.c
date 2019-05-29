#include "hrbstd.h"

void win_put_str(int window, char *window_buf, int x, int y, char color, unsigned char *str);
void wait(int limit, int timer, char *keyflag);

// キャラの描画用
// - abcd : 敵
// - efg : 自機
// - h : 弾
static unsigned char charset[16 * 8] = {

    /* invader(0) */
    0x00, 0x00, 0x00, 0x43, 0x5f, 0x5f, 0x5f, 0x7f,
    0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x20, 0x3f, 0x00,

	/* invader(1) */
    0x00, 0x0f, 0x7f, 0xff, 0xcf, 0xcf, 0xcf, 0xff,
    0xff, 0xe0, 0xff, 0xff, 0xc0, 0xc0, 0xc0, 0x00,

	/* invader(2) */
    0x00, 0xf0, 0xfe, 0xff, 0xf3, 0xf3, 0xf3, 0xff,
    0xff, 0x07, 0xff, 0xff, 0x03, 0x03, 0x03, 0x00,

	/* invader(3) */
    0x00, 0x00, 0x00, 0xc2, 0xfa, 0xfa, 0xfa, 0xfe,
    0xf8, 0xf8, 0xf8, 0xf8, 0x00, 0x04, 0xfc, 0x00,

	/* fighter(0) */
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x43, 0x47, 0x4f, 0x5f, 0x7f, 0x7f, 0x00,

	/* fighter(1) */
    0x18, 0x7e, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xff,
    0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x00,

	/* fighter(2) */
    0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0xc2, 0xe2, 0xf2, 0xfa, 0xfe, 0xfe, 0x00,

	/* laser */
    0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00
};

void HariMain(){
    int window, timer, me_x, laserwait, laser_x = 0, laser_y;
    int inv_x, inv_y, movewait, movewait_init, inv_dir;
    int inv_line, score, highscore, point;
    char win_buf[336 * 261], keyflag[4], inv_status[32 * 6], score_str[10];
    static char inv_status_str[32] = " abcd abcd abcd abcd abcd ";

    // 初期設定
    window = api_gen_window(win_buf, 336, 261, -1, "invador");
    timer = api_alloc_timer();
    api_boxfill_window(window, 6, 27, 329, 254, 0);
    api_init_timer(timer, 128);

    // スコア設定
    highscore = 0;
    win_put_str(window, win_buf, 22, 0, 7, "HIGH:0       ");

init_game:
    score = 0;
    point = 1;
    win_put_str(window, win_buf, 4, 0, 7, "SCORE:0       ");
    movewait_init = 20;
    me_x = 18;
    win_put_str(window, win_buf, me_x, 13, 6, "efg");
    printf("3...");
    wait(100, timer, keyflag);

next_group:
    printf("2...");
    wait(100, timer, keyflag);

    // 敵配置
    inv_x = 7, inv_y = 1, inv_line = 6;
    for(int y = 0; y < 6; ++ y){
        for(int x = 0; x < 27; ++x){
            inv_status[y * 32 + x] = inv_status_str[x];
        }
    }
    keyflag[0] = 0;
    keyflag[1] = 0;
    keyflag[2] = 0;

    // プラズマ砲
    laser_y = 0;
    laserwait = 0;
    movewait = movewait_init;
    inv_dir = 1;
    printf("1...");
    wait(100, timer, keyflag);
    printf("START!!\n");

    // メインループ
    while(1){
        if(laserwait != 0){
            -- laserwait;
            keyflag[2] = 0;
        }

        wait(4, timer, keyflag);

        // 自機の処理
        if(keyflag[0] != 0 && me_x > 0){
            -- me_x;
            win_put_str(window, win_buf, me_x, 13, 6, "efg ");
            keyflag[0] = 0;
        }
        else if(keyflag[1] != 0 && me_x < 37){
            win_put_str(window, win_buf, me_x, 13, 6, " efg");
            ++ me_x;
            keyflag[1] = 0;
        }
        else if(keyflag[2] != 0 && laserwait == 0){
            laserwait = 15;
            laser_x = me_x + 1;
            laser_y = 13;
        }

        // 敵移動
        if(movewait != 0){
            -- movewait;
        }else{
            movewait = movewait_init;
            if(inv_x + inv_dir > 14 || inv_x + inv_dir < 0){    // 敵が画面端
                if(inv_y + inv_line == 13){
                    break;      // GAMEOVER
                }
                inv_dir *= -1;
                win_put_str(window, win_buf, inv_x + 1, inv_y, 0, "                         ");
                ++ inv_y;
            }else{
                inv_x += inv_dir;
            }
            for(int idx = 0; idx < inv_line; ++ idx){
                win_put_str(window, win_buf, inv_x, inv_y + idx, 2, inv_status + idx * 32);
            }
        }

        // レーザ処理
        if(laser_y > 0){
            if(laser_y < 13){
                if(inv_x < laser_x && laser_x < inv_x + 25 && inv_y <= laser_y && laser_y < inv_y + inv_line){
                    win_put_str(window, win_buf, inv_x, laser_y, 2, inv_status + (laser_y - inv_y) * 32);
                }else{
                    win_put_str(window, win_buf, laser_x, laser_y, 0, " ");
                }
            }

            // レーザ移動
            -- laser_y;
            if(laser_y > 0){
                win_put_str(window, win_buf, laser_x, laser_y, 3, "h");
            }else{
                point = max(1, point - 10);
            }

            // 当たり判定
            if(inv_x < laser_x && laser_x < inv_x + 25 && inv_y <= laser_y && laser_y < inv_y + inv_line){
                char *inv_p = inv_status + (laser_y - inv_y) * 32 + (laser_x - inv_x);
                if(*inv_p != ' '){  // HIT
                    // スコア計算
                    score += point;
                    ++ point;
                    sprintf(score_str, "%d", score);
                    win_put_str(window, win_buf, 10, 0, 7, score_str);
                    if(highscore < score){
                        highscore = score;
                        win_put_str(window, win_buf, 27, 0, 7, score_str);
                    }

                    // 敵再描画
                    for(-- inv_p; *inv_p != ' '; -- inv_p);
                    for(int idx = 1; idx < 5; ++ idx){
                        inv_p[idx] = ' ';
                    }
                    win_put_str(window, win_buf, inv_x, laser_y, 2, inv_status + (laser_y - inv_y) * 32);

                    // 終了判定
                    char continue_flag = 0;
                    for(; inv_line > 0; -- inv_line){
                        for(char *p = inv_status + (inv_line - 1) * 32; *p != 0; ++ p){
                            if(*p != ' '){
                                goto alive;
                            }
                        }
                    }
                    movewait_init -= movewait_init / 3;
                    goto next_group;
alive:
                    laser_y = 0;
                }
            }
        }
    }

    // GAMEOVER
    win_put_str(window, win_buf, 15, 6, 1, "GAME OVER");
    wait(0, timer, keyflag);
    for(int idx = 1; idx < 14; ++ idx){
        win_put_str(window, win_buf, 0, idx, 0, "                                        ");
    }
    goto init_game;
}

// 文字列 or キャラクター描画
void win_put_str(int window, char *win_buf, int x, int y, char color, unsigned char *str){
    // 座標設定
    int str_len = strlen(str);
    int x0 = x * 8 + 8;
    x = x * 8 + 8;
    y = y * 16 + 29;

    // 描画対象範囲を塗りつぶす
    api_boxfill_window(window + 1, x, y, x + str_len * 8 - 1, y + 15, 0);

    // フォント描画
    char *buf_p = win_buf + y * 336, *font_p;
    char t[2] = {0, 0};
    while(1){
        int char_id = *str;
        if(char_id == 0){                                          // 文字列終わり
            break;
        }
        if(char_id != ' '){
            if('a' <= char_id && char_id <= 'h'){                      // 敵キャラ描画
                font_p = charset + 16 * (char_id - 'a');
                buf_p += x;
                for(int idx = 0; idx < 16; ++ idx){
                    if((font_p[idx] & 0x80) != 0){ buf_p[0] = color; }
                    if((font_p[idx] & 0x40) != 0){ buf_p[1] = color; }
                    if((font_p[idx] & 0x20) != 0){ buf_p[2] = color; }
                    if((font_p[idx] & 0x10) != 0){ buf_p[3] = color; }
                    if((font_p[idx] & 0x08) != 0){ buf_p[4] = color; }
                    if((font_p[idx] & 0x04) != 0){ buf_p[5] = color; }
                    if((font_p[idx] & 0x02) != 0){ buf_p[6] = color; }
                    if((font_p[idx] & 0x01) != 0){ buf_p[7] = color; }
                    buf_p += 336;
                }
                buf_p -= 336 * 16 + x;
            }else{                                                  // 通常文字描画
                t[0] = *str;
                api_putstr_window(window + 1, x, y, color, str_len, t);
            }
        }
        ++ str;
        x += 8;
    }

    api_refresh_window(window, x0, y, x, y + 16);
    return;
}

void wait(int limit, int timer, char *keyflag){
    // タイマ設定
    int wait_key;
    if(limit > 0){
        api_set_timer(timer, limit);
        wait_key = 128;
    }else{
        wait_key = 0x0a;
    }

    // キー待ち
    while(1){
        int inp = api_keyinput(1);
        if(inp == wait_key){ break; }
        else if(inp == '4'){ keyflag[0] = 1; }
        else if(inp == '6'){ keyflag[1] = 1; }
        else if(inp == ' '){ keyflag[2] = 1; }
    }
    return;
}