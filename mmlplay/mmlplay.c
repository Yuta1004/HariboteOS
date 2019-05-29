#include "hrbstd.h"

void wait_timer(int timer, int limit);
void app_end(char *message);

int HariMain(){
    char win_buf[256 * 112], txt_buf[100 * 1024], file_name[30];
    char *fn_p;
    int window, fn_length, timer, def_length = 192 / 4, tempo = 120, q_pal = 7;

    // 音番号と周波数の対応表
    // tone_tableはオクターブ16のリスト
    static int tone_table[12] = {
        1071618315, 1135340056, 1202850889, 1274376125, 1350154473, 1430438836,
        1515497155, 1605613306, 1701088041, 1802240000, 1909406767, 2022946002
    };
    static int note_table[7] = { 9, 11, 0, 2, 3, 5, 7 };

    // 引数取り出し
    api_get_command(file_name, 30);
    for(fn_p = file_name; *fn_p > ' '; ++ fn_p);
    for(; *fn_p == ' '; ++ fn_p);
    fn_length = strlen(fn_p);
    if(fn_length > 12){
        app_end("File Open Error!\n");
    }else if(fn_length == 0){
        app_end(0);
    }

    // ウィンドウ生成
    window = api_gen_window(win_buf, 256, 112, -1, "mmlplay");
    api_putstr_window(window, 128, 32, 0, fn_length, fn_p);
    api_boxfill_window(window, 8, 60, 247, 76, 7);
    api_boxfill_window(window, 6, 86, 249, 105, 7);
    api_refresh_window(window, 0, 0, 256, 112);

    // MMLファイル読み込み
    int fhandle = api_fopen(fn_p), f_size;
    if(fhandle == 0){
        app_end("File Not Found Error!\n");
    }
    f_size = api_fsize(fhandle, 0);
    f_size = min(100 * 1204 - 1, f_size);
    api_fread(fhandle, txt_buf, f_size);
    api_fclose(fhandle);

    // MMLファイル前処理
    char *mml_p = txt_buf;
    int mode = 0;
    txt_buf[f_size] = 0;
    for(char *p = txt_buf; *p != 0; ++ p){
        if(mode == 0 && *p > ' '){                       // スペース・改行ではない
            if(*p == '/'){              // コメント始点
                if(p[1] == '*'){
                    mode = 1;
                }else if(p[1] == '/'){
                    mode = 2;
                }else{
                    *mml_p = *p;
                    if('a' <= *p && *p <= 'z'){
                        *mml_p += 'A' - 'a';
                    }
                    ++ mml_p;
                }
            }
            else if(*p == 0x22){        // 0x22
                *mml_p = *p;
                ++ mml_p;
                mode = 3;
            }
            else{
                *mml_p = *p;
                ++ mml_p;
            }
        }
        else if(mode == 1 && *p == '*' && p[1] == '/'){     // 複数行コメント終端
            ++ p;
            mode = 0;
        }
        else if(mode == 2 && *p == 0x0a){                   // 1行コメント終端
            mode = 0;
        }
        else if(mode == 3){                                  // 普通の文字
            *mml_p = *p;
            ++ mml_p;
            if(*p == 0x22){
                mode = 0;
            }else if(*p == '%'){
                ++ p;
                *mml_p = *p;
                ++ mml_p;
            }
        }
    }
    *mml_p = 0;

    // タイマ初期化
    timer = api_alloc_timer();
    api_init_timer(timer, 128);

    // メインループ
    char *p = txt_buf, str[30];
    int tone, old_tone = 0, octave = 4;
    while(1){
        if(('A' <= *p && *p <= 'Z') || *p == 'R'){          // 音符・休符
            // 周波数計算
            if(*p == 'R'){
                tone = 0;
                str[0] = 0;
            }else{
                tone = octave * 12 + note_table[*p - 'A'] + 12;
                str[0] = 'O';
                str[1] = '0' + octave;
                str[2] = *p;
                str[3] = ' ';
                str[4] = 0;
            }

            // オクターブ調節
            ++ p;
            if(*p == '+' || *p == '-' || *p == '#'){
                str[3] = *p;
                if(*p == '-'){
                    -- tone;
                }else{
                    ++ tone;
                }
                ++ p;
            }

            // 再生 & 情報描画
            if(tone != old_tone){
                // 曲情報
                api_boxfill_window(window + 1, 32, 36, 63, 51, 8);
                if(str[0] != 0){
                    api_putstr_window(window + 1, 32, 36, 10, 4, str);
                }
                api_refresh_window(window, 32, 36, 64, 52);

                // 再生音情報
                if(28 <= old_tone && old_tone <= 107){
                    api_boxfill_window(window, (old_tone - 28) * 3 + 8, 60, (old_tone - 28) * 3 + 10, 76, 7);
                }
                if(28 <= tone && tone <= 107){
                    api_boxfill_window(window, (tone - 28) * 3 + 8, 60, (tone - 28) * 3 + 10, 76, 4);
                }

                // 再生
                if(str[0] != 0){
                    api_beep(tone_table[tone % 12] >> (17 - tone / 12));
                }else{
                    api_beep(0);
                }
                old_tone = tone;
            }

            // 音長計算
            int length;
            if('0' <= *p && *p <= '9'){
                length = 192 / strtol(p, &p, 10);
            }else{
                length = def_length;
            }
            for(; *p == '.';){
                ++ p;
                length += length / 2;
            }
            length *= (60 * 100 / 48);
            length /= tempo;

            // 音長だけ待って再生を停止する
            int wait = 0;
            if(str[0] != 0 && q_pal < 8 && *p != '&'){
                wait = length * q_pal / 8;
                wait_timer(timer, wait);
                api_boxfill_window(window, 32, 36, 63, 51, 8);
                if(28 <= old_tone && old_tone <= 107){
                    api_boxfill_window(window, (old_tone - 28) * 3 + 8, 60, (old_tone - 28) * 3 + 10, 76,7);
                }
                old_tone = 0;
                api_beep(0);
            }else{
                wait = 0;
                if(*p == '&'){
                    ++ p;
                }
            }
            wait_timer(timer, 1 - wait);
        }
        else if(*p == '<'){         // オクターブ --
            ++ p;
            -- octave;
        }
        else if(*p == '>'){         // オクターブ ++
            ++ p;
            ++ octave;
        }
        else if(*p == 'O'){         // オクターブ指定
            octave = strtol(p + 1, &p, 10);
        }
        else if(*p == 'Q'){         // Qパラメータ
            q_pal = strtol(p + 1, &p, 10);
        }
        else if(*p == 'L'){         // デフォルト音長設定
            def_length = strtol(p + 1, &p, 10);
            if(def_length == 0){
                app_end("Syntax Error!\n");
            }
            def_length = 192 / def_length;
            for(; *p == '.'; ){
                ++ p;
                def_length += def_length / 2;
            }
        }
        else if(*p == 'T'){         // テンポ指定
            tempo = strtol(p + 1, &p, 10);
        }
        else if(*p == '$'){         // 拡張コマンド
            if(p[1] == 'K'){        // カラオケコマンド
                // 歌詞読み込み
                char lyric[70];
                p += 2;
                for(; *p != 0x22; ++ p){    // "
                    if(*p == 0){
                        app_end("Syntax Error! 1\n");
                    }
                }
                ++ p;
                int cnt = 0;
                for(; cnt < 32; ++ cnt){
                    if(*p == 0){
                        app_end("Syntax Error! 2\n");
                    }
                    if(*p == 0x22){
                        break;
                    }
                    if(*p == '%'){      // 2バイト文字
                        lyric[cnt] == p[cnt];
                        p += 2;
                    }else{              // 1バイト文字
                        lyric[cnt] = *p;
                        ++ p;
                    }
                }

                // 歌詞表示
                if(cnt > 30){
                    app_end("Lyric too long!\n");
                }
                api_boxfill_window(window + 1, 8, 88, 247, 103, 7);
                lyric[cnt] = 0;
                if(cnt != 0){
                    api_putstr_window(window + 1, 128 - cnt * 4, 88, 0, cnt, lyric);
                }
                api_refresh_window(window, 8, 88, 248, 104);
            }

            // 残りの文字をスキップ
            for(; *p != ';'; ++p){
                if(*p == 0){
                    app_end("Syntax Error! 3\n");
                }
            }
            ++ p;
        }
        else if(*p == 0){       // 再生終了
            p = txt_buf;
        }
        else{
            app_end("Syntax Error! 4\n");
        }
    }
}


void wait_timer(int timer, int limit){
    api_set_timer(timer, limit);
    while(1){
        int key = api_keyinput(1);
        if(key == 'Q' || key == 'q'){   // 終了コマンド
            app_end(0);
        }
        if(key == 128){     // タイマ
            return;
        }
    }
}

void app_end(char *message){
    if(message != 0){
        api_putstr(message);
    }
    api_beep(0);
    exit();
}