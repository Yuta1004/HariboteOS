#include "hrbstd.h"

void HariMain(){
    static char str[9] = {0xb2, 0xdb, 0xca, 0xc6, 0xce, 0xcd, 0xc4, 0x0a, 0x00};
    static char *str_j = "ｲﾛﾊﾆﾎﾍﾄ\n";
    api_putstr(str);
    api_end();
}