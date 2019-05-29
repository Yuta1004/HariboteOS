#include "hrbstd.h"

/*  strcmp  */
char strcmp(char str_a[], char str_b[]){
    // 文字列同士の大小を比較
    int idx = 0;
    for(; str_a[idx] != '\0' && str_b[idx] != '\0'; ++ idx){
        if(str_a[idx] > str_b[idx]){
            return 1;
        }else if(str_a[idx] < str_b[idx]){
            return -1;
        }
    }

    // 比較が中断されたら
    if(str_a[idx] == '\0' && str_b[idx] == '\0'){
        return 0;
    }else if(str_a[idx] == '\0'){
        return -1;
    }else{
        return 1;
    }
}

/*  strncmp */
char strncmp(char str_a[], char str_b[], int size){
    // 文字列同士の大小を比較
    int idx = 0;
    for(; str_a[idx] != '\0' && str_b[idx] != '\0' && idx < size; ++ idx){
        if(str_a[idx] > str_b[idx]){
            return 1;
        }else if(str_a[idx] < str_b[idx]){
            return -1;
        }
    }

    // 比較が中断されたら
    if(idx == size){
        return 0;
    }else if(str_a[idx] == '\0'){
        return -1;
    }else if(str_b[idx] == '\0'){
        return 1;
    }
}

/* strlen */
int strlen(char *str){
    int length = 0;
    while(*str++ != '\0'){
        ++ length;
    }
    return length;
}

/* strtol */
long strtol(char *nptr, char **endptr, int base){
    // baseチェック
    if(base != 10 && base != 16 && base != 0){
        return -1;
    }

    // 空白を読み進める
    for(; *nptr == ' '; ++ nptr);

    // baseが0だった場合
    if(nptr[0] == '0' && nptr[1] == 'x'){
        base = 16;
        nptr += 2;
    }else{
        base = 10;
    }

    // 変換
    int val = 0;
    for(; *nptr != '\0' && *nptr != ' ' && *nptr != 0; ++ nptr){
        if('0' <= *nptr && *nptr <= '9'){
            val = val * base + (*nptr - '0');
        }else if('a' <= *nptr && *nptr <= 'f' && base == 16){
            val = val * base + (*nptr - 'a' + 10);
        }else{
            break;
        }
    }

    *endptr = nptr;
    return (long)val;
}