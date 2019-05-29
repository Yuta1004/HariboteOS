#include "hrbstd.h"

#define INVALID     -0x7fffffff

char *skip_space(char *p);
int calc(char **p, int prioriry);

void HariMain(){
    // コマンドライン取得
    char command_line[30], *c_p;
    api_get_command(command_line, 30);
    c_p = command_line;

    // 計算
    for(; *c_p > ' '; ++ c_p);
    int calc_result = calc(&c_p, 9);

    // 出力
    char str[30];
    if(calc_result == INVALID){
        printf("Syntax Error!!\n");
    }else{
        sprintf(str, "= %d = 0x%x\n", calc_result, calc_result);
        api_putstr(str);
    }

    exit();
}

char *skip_space(char *p){
    for(; *p == ' '; ++ p);
    return p;
}

// 再帰的に数式を計算する
int calc(char **pp, int priority){
    char *p = *pp;
    int num = INVALID, tmp;
    p = skip_space(p);

    // 単項演算子
    if(*p == '+'){
        p = skip_space(p + 1);
        num = calc(&p, 0);
    }
    else if(*p == '-'){
        p = skip_space(p + 1);
        num = calc(&p, 0);
        if(num != INVALID){
            num = -num;
        }
    }
    else if(*p == '~'){
        p = skip_space(p + 1);
        num = calc(&p, 0);
        if(num != INVALID){
            num = ~num;
        }
    }
    else if(*p == '('){
        p = skip_space(p + 1);
        num = calc(&p, 9);
        if(*p == ')'){
            p = skip_space(p + 1);
        }else{
            num = INVALID;
        }
    }
    else if('0' <= *p && *p <= '9'){
        num = strtol(p, &p, 0);
    }
    else{
        num = INVALID;
    }

    // 二項演算子
    while(1){
        if(num == INVALID){
            break;
        }
        p = skip_space(p);

        if(*p == '+' && priority > 2){
            p = skip_space(p + 1);
            tmp = calc(&p, 2);
            if(tmp != INVALID){
                num += tmp;
            }else{
                num = INVALID;
            }
        }
        else if(*p == '-' && priority > 2){
            p = skip_space(p + 1);
            tmp = calc(&p, 2);
            if(tmp != INVALID){
                num -= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(*p == '*' && priority > 1){
            p = skip_space(p + 1);
            tmp = calc(&p, 1);
            if(tmp != INVALID){
                num *= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(*p == '/' && priority > 1){
            p = skip_space(p + 1);
            tmp = calc(&p, 1);
            if(tmp != INVALID){
                num /= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(*p == '%' && priority > 1){
            p = skip_space(p + 1);
            tmp = calc(&p, 1);
            if(tmp != INVALID){
                num %= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(p[0] == '<' && p[1] == '<' && priority > 3){
            p = skip_space(p + 2);
            tmp = calc(&p, 3);
            if(tmp != INVALID){
                num <<= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(p[0] == '>' && p[1] == '>' && priority > 3){
            p = skip_space(p + 2);
            tmp = calc(&p, 3);
            if(tmp != INVALID){
                num >>= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(*p == '&' && priority > 4){
            p = skip_space(p + 1);
            tmp = calc(&p, 4);
            if(tmp != INVALID){
                num &= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(*p == '^' && priority > 5){
            p = skip_space(p + 1);
            tmp = calc(&p, 5);
            if(tmp != INVALID){
                num ^= tmp;
            }else{
                num = INVALID;
            }
        }
        else if(*p == '|' && priority > 6){
            p = skip_space(p + 1);
            tmp = calc(&p, 6);
            if(tmp != INVALID){
                num |= tmp;
            }else{
                num = INVALID;
            }
        }
        else{
            break;
        }
    }

    p = skip_space(p);
    *pp = p;
    return num;
}