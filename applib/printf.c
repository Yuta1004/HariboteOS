#include "hrbstd.h"
#include <stdarg.h>

void printf(char *fmt, ...) {
    va_list list;
    int i, len, str_idx = 0;
    char str[1000], converted_str[1000];
    va_start(list, fmt);

    while (*fmt) {
        if(*fmt == '%') {
            fmt ++;
            switch(*fmt){
                case 'd':
                    len = dec2asc(converted_str, va_arg(list, int));
                    break;
                case 'x':
                    len = hex2asc(converted_str, va_arg(list, int));
                    break;
            }
            for(int idx = 0; converted_str[idx] != 0; ++ idx){
                str[str_idx] = converted_str[idx];
                str_idx ++;
            }
            fmt ++;
        } else {
            str[str_idx] = *(fmt ++);
            str_idx ++;
        }
    }
    str[str_idx] = 0;

    api_putstr(str);
    va_end(list);
}
