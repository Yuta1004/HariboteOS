#include "bootpack.h"

// マウス有効化
void enable_mouse(struct FIFO32 *fifo, int data, struct MOUSE_DEC *mdec){
    mouseinfo = fifo;
    mouse_base = data;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;
}

// マウスデータを色々する
int mouse_decode(struct MOUSE_DEC *mdec, int data){
    // データ受信準備
    if(mdec->phase == 0){
        if(data == 0xfa){
            mdec->phase = 1;
        }
        return 0;
    }

    // 1バイト目受信
    if(mdec->phase == 1){
        // 正しい1バイト目かどうか検証
        if((data & 0xc8) == 0x08){
            mdec->buf[0] = data;
            mdec->phase = 2;
        }
        return 0;
    }

    // 2バイト目受信
    if(mdec->phase == 2){
        mdec->buf[1] = data;
        mdec->phase = 3;
        return 0;
    }

    // 3バイト目受信
    if(mdec->phase == 3){
        mdec->buf[2] = data;
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];

        if((mdec->buf[0] & 0x10) != 0){
            mdec->x |= 0xffffff00;
        }
        if((mdec->buf[0] & 0x20) != 0){
            mdec->y |= 0xffffff00;
        }

        mdec->y *= -1;  // マウスはy座標の符号が逆
        return 1;
    }

    return -1;
}

