#include "hrbstd.h"

#define MAX 100000

void HariMain(){
    int flag[MAX];
    for(int idx = 0; idx < MAX; ++ idx){
        flag[idx] = 0;
    }

    char str[10];
    for(int num = 2; num < MAX; ++ num){
        if(flag[num] == 0){
            printf("%d ", num);
            for(int check_num = num * 2; check_num < MAX; check_num += num){
                flag[check_num] = 1;
            }
        }
    }

    api_putchar('\n');
    api_end();
}