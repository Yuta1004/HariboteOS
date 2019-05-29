#include "hrbstd.h"

//10進数からASCIIコードに変換
int dec2asc (char *str, int dec) {
    int len = 0, len_buf; //桁数
    int buf[10];
    char minus_flag = 0;

    // マイナスチェック
    if(dec < 0){
        dec *= -1;
        minus_flag = 1;
    }

    //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
    while (1) {
        buf[len++] = dec % 10;
        if (dec < 10) break;
        dec /= 10;
    }

    // 出力
    len_buf = len;
    if(minus_flag){
        *(str++) = '-';
    }
    while (len) {
        *(str++) = buf[--len] + 0x30;
    }
    return len_buf + (int)minus_flag;
}

//16進数からASCIIコードに変換
int hex2asc (char *str, int dec) { //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
    int len = 0, len_buf; //桁数
    int buf[10];
    while (1) {
        buf[len++] = dec % 16;
        if (dec < 16) break;
        dec /= 16;
    }
    len_buf = len;
    while (len) {
        len --;
        *(str++) = (buf[len]<10)?(buf[len] + 0x30):(buf[len] - 9 + 0x60);
    }
    return len_buf;
}