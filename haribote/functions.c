#include "bootpack.h"

/*  strcmp  */
char mstrcmp(char str_a[], char str_b[]){
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
char mstrncmp(char str_a[], char str_b[], int size){
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

/*  min  */
int min(int comp_a, int comp_b){
    if(comp_a > comp_b){
        return comp_b;
    }else{
        return comp_a;
    }
}

/*  max  */
int max(int comp_a, int comp_b){
    if(comp_a >= comp_b){
        return comp_a;
    }else{
        return comp_b;
    }
}
