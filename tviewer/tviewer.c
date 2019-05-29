#include "hrbstd.h"

void arg_error();
void file_404_error();
char *skip_space(char *p);
void text_view(int window, int width, int height, int x_skip, char *fp, int tab_size, int lang);
char *line_view(int window, int width, int y, int x_skip, unsigned char *fp, int tab_size, int lang);
int puttab(int x, int width, int x_skip, char *str, int tab_size);


void HariMain(){
    char win_buf[1024 * 757], txt_buf[240 * 1024];
    int width = 70, height = 30, tab_size = 4, speed_x = 1, speed_y = 1;
    int window, lang, x_skip = 0;
    char *fn_p = 0, *fp;

    // コマンドライン取得
    char command_line[30], *c_p;
    api_get_command(command_line, 30);
    c_p = command_line;
    for(; *c_p > ' '; ++ c_p);

    // 引数取り出し
    while(*c_p != 0){
        c_p = skip_space(c_p);
        if(*c_p == '-'){        // オプション(w, h, t)
            if(c_p[1] == 'w'){
                width = strtol(c_p + 2, &c_p, 0);
                width = min(126, max(20, width));
            }
            else if(c_p[1] == 'h'){
                height = strtol(c_p + 2, &c_p, 0);
                height = min(45, max(1, height));
            }
            else if(c_p[1] == 't'){
                tab_size = strtol(c_p + 2, &c_p, 0);
                if(tab_size < 1){
                    tab_size = 4;
                }
            }
            else{
                arg_error();
            }
        }else{                  // ファイル名
            if(fn_p != 0){
                arg_error();
            }
            fn_p = c_p;
            for(; *c_p > ' '; ++ c_p);
        }
    }
    if(fn_p == 0){
        arg_error();
    }

    // ウィンドウ生成
    window = api_gen_window(win_buf, width * 8 + 16, height * 16 + 37, -1, "TextViewer");
    api_boxfill_window(window, 6, 27, width * 8 + 9, height * 16 + 30, 7);

    // ファイル読み込み
    int file_handle, file_size;
    file_handle = api_fopen(fn_p);
    if(file_handle == 0){
        file_404_error();
    }
    file_size = api_fsize(file_handle, 0);
    file_size = min(240 * 1204 - 2, file_size);
    txt_buf[0] = 0x0a;
    api_fread(file_handle, txt_buf + 1, file_size);
    api_fclose(file_handle);
    txt_buf[file_size + 1] = 0;

    // ファイルに対して前処理をする
    fp = txt_buf + 1;
    for(char *p = txt_buf + 1; *p != 0; ++ p){
        if(*p != 0x0d){
            *fp = *p;
            ++ fp;
        }
    }
    *fp = 0;

    // メインループ
    lang = api_check_langmode();
    fp = txt_buf + 1;
    while(1){
        // ファイル内容描画
        text_view(window, width, height, x_skip, fp, tab_size, lang);

        // キー操作(コマンド)
        int key = api_keyinput(1);
        if(key == 'Q' || key == 'q'){
            exit();
        }
        else if('A' <= key && key <= 'F'){
            speed_x = 1 << (key - 'A');
        }
        else if('a' <= key && key <= 'f'){
            speed_y = 1 << (key - 'a');
        }
        else if(key == '<' && tab_size > 1){
            tab_size /= 2;
        }
        else if(key == '>' && tab_size < 256){
            tab_size *= 2;
        }

        // キー操作(カーソル)
        else if(key == 'h'){
            while(1){
                x_skip = max(0, x_skip - speed_x);
                if(api_keyinput(0) != 'h'){
                    break;
                }
            }
        }
        else if(key == 'l'){
            while(1){
                x_skip += speed_x;
                if(api_keyinput(0) != 'l'){
                    break;
                }
            }
        }
        else if(key == 'k'){
            while(1){
                for(int idx = 0; idx < speed_y && fp != txt_buf + 1; ++ idx){
                    for(-- fp; fp[-1] != 0x0a; -- fp);
                }
                if(api_keyinput(0) != 'k'){
                    break;
                }
            }
        }
        else if(key == 'j'){
            while(1){
                for(int idx = 0; idx < speed_y; ++ idx){
                    char *q;
                    for(q = fp; *q != 0 && *q != 0x0a; ++ q);
                    if(*q == 0){
                        break;
                    }
                    fp = q + 1;
                }
                if(api_keyinput(0) != 'j'){
                    break;
                }
            }
        }
    }
}

void arg_error(){
    printf("Usage : tviewer sjis.txt -w30 -h10 -t4\n");
    exit();
}

void file_404_error(){
    printf("File Not Found...\n");
    exit();
}

char *skip_space(char *p){
    for(; *p == ' '; ++ p);
    return p;
}

void text_view(int window, int width, int height, int x_skip, char *fp, int tab_size, int lang){
    api_boxfill_window(window + 1, 8, 29, width * 8 + 7, height * 16 + 28, 7);
    for(int y = 0; y < height; ++ y){
        fp = line_view(window, width, y * 16 + 29, x_skip, fp, tab_size, lang);
    }
    api_refresh_window(window, 8, 29, width * 8 + 8, height * 16 + 29);
    return;
}

char *line_view(int window, int width, int y, int x_skip, unsigned char *fp, int tab_size, int lang){
    int x = -x_skip;
    char s[130];

    while(1){
        if(*fp == 0){
            break;
        }
        if(*fp == 0x0a){
            ++ fp;
            break;
        }

        // ASCII
        if(lang == 0){
            if(*fp == 0x09){
                x = puttab(x, width, x_skip, s, tab_size);
            }else{
                if(0 <= x && x < width){
                    s[x] = *fp;
                }
                ++ x;
            }
            ++ fp;
        }

        // SJIS
        if(lang == 1){
            if(*fp == 0x09){
                x = puttab(x, width, x_skip, s, tab_size);
                ++ fp;
            }
            else if((0x81 <= *fp && *fp <= 0x9f) || (0xe0 <= *fp && *fp <= 0xfc)){      // 全角文字
                if(x == -1){
                    s[0] = ' ';
                }
                else if(0 <= x && x < width - 1){
                    s[x] = *fp;
                    s[x + 1] = fp[1];
                }
                else if(x == width - 1){
                    s[x] = ' ';
                }
                fp += 2;
                x += 2;
            }
            else{
                if(0 <= x && x <= width){
                    s[x] = *fp;
                }
                ++ fp;
                ++ x;
            }
        }

        // EUC
        if(lang == 2){
            if(*fp == 0x09){
                x = puttab(x, width, x_skip, s, tab_size);
                ++ fp;
            }
            else if(0xa1 <= *fp && *fp <= 0xfe){        // 全角文字
                if(x == -1){
                    s[x] = ' ';
                }
                else if(0 <= x && x < width - 1){
                    s[x] = *fp;
                    s[x + 1] = fp[1];
                }
                else if(x == width - 1){
                    s[x] = ' ';
                }
                fp += 2;
                x += 2;
            }
            else{
                if(0 <= x && x < width){
                    s[x] = *fp;
                }
                ++ fp;
                ++ x;
            }
        }
    }

    // テキスト描画（更新なし)
    x = min(x, width);
    if(x > 0){
        s[x] = 0;
        api_putstr_window(window + 1, 8, y, 0, x, s);
    }

    return fp;
}

int puttab(int x, int width, int x_skip, char *s, int tab_size){
    while(1){
        if(0 <= x && x < width){
            s[x] = ' ';
        }
        ++ x;
        if((x + x_skip) % tab_size == 0){
            break;
        }
    }

    return x;
}