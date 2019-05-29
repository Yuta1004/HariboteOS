#include "hrbstd.h"

struct DLL_STRPICENV {
    int work[64 * 1024 / 4];
    char _tmp[1024 * 512];
};

struct RGB{
    unsigned char b, g, r, t;
};

/* jpeg.c */
int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, unsigned char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, unsigned char *fp, int b_type, unsigned char *buf, int skip);

/* bmp.asm */
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

void app_end(char *message);
unsigned char rgb2pal(int r, int g, int b, int x, int y);

int HariMain(){
    struct DLL_STRPICENV env;
    char file_buf[512 * 1024], win_buf[1040 * 805], command[30];
    char *fn_p;
    int window, width, height;
    struct RGB pic_data[1024 * 768];

    // 引数取り出し
    api_get_command(command, 30);
    for(fn_p = command; *fn_p > ' '; ++ fn_p);
    for(; *fn_p == ' '; ++ fn_p);

    // ファイル読み込み
    int fhandle = api_fopen(fn_p), f_size;
    if(fhandle == 0){
        app_end("File not found...\n");
    }
    f_size = api_fsize(fhandle, 0);
    if(f_size > 512 * 1024){
        app_end("File too large\n");
    }
    api_fread(fhandle, file_buf, f_size);
    api_fclose(fhandle);

    // ファイルタイプチェック
    // ファイルタイプが JPEG or BMP だったならinfoに以下の情報が格納される
    // - info[0] : ファイルタイプ (1 : BMP, 2: JPEG)
    // - info[1] : カラー情報
    // - info[2] : width
    // - info[3] : height
    int info[8];
    if(info_JPEG(&env, info, f_size, file_buf) == 0){
        if(info_BMP(&env, info, f_size, file_buf) == 0){
            app_end("File type unknown\n");
        }
    }
    if(info[2] > 1024 || info[3] > 768){
        app_end("Picture too large\n");
    }

    // ウィンドウ生成
    width = max(136, info[2] + 16);
    height = info[3];
    window = api_gen_window(win_buf, width, height + 37, -1, "ImageView");

    // ファイルを画像データに変換
    int dec_result;
    if(info[0] == 1){   // BMP
        dec_result = decode0_BMP(&env, f_size, file_buf, 4, (char *) pic_data, 0);
    }else{
        dec_result = decode0_JPEG(&env, f_size, file_buf, 4, (char *) pic_data, 0);
    }
    if(dec_result != 0){
        app_end("Decode error\n");
    }

    // 描画
    char *win_buf_p;
    struct RGB *pic_data_p;
    for(int y = 0; y < height; ++ y){
        win_buf_p = win_buf + (y + 29) * width + (width - info[2]) / 2;
        pic_data_p = pic_data + y * info[2];
        for(int x = 0; x < info[2]; ++ x){
            win_buf_p[x] = rgb2pal(pic_data_p[x].r, pic_data_p[x].g, pic_data_p[x].b, x, y);
        }
    }
    api_refresh_window(window, 0, 0, width, height + 37);

    // キー入力で終了
    api_keyinput(1);
    exit();
}

void app_end(char *message){
    if(message != 0){
        printf(message);
    }
    exit();
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

