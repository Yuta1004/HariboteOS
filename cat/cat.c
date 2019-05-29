#include "hrbstd.h"

void HariMain(){
    char str[40];

    // ファイル名をコマンドラインから取得
    char command[40], *command_p;
    api_get_command(command, 40);
    for(command_p = command; *command_p > ' '; ++ command_p);    // スペースまで読み飛ばす(コマンド名無視)
    for(; *command_p == ' '; ++ command_p);                      // スペースを読み飛ばす(区切り無視)

    // ファイルを開く
    int fhandle = api_fopen(command_p);
    char read_c;
    if(fhandle != 0){
        while(1){
            if(api_fread(fhandle, &read_c, 1) == 0){
                break;
            }
            api_putchar(read_c);
        }
    }else{
        printf("File Not Found...\n");
        api_putstr(str);
    }

    api_putchar('\n');
    api_end();
}