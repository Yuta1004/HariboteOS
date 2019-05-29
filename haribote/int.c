#include "bootpack.h"
#define PORT_KEYDAT 0x0060

// PIC(Programmable Inturrept Controller)の設定
void init_pic(void){
    io_out8(PIC0_IMR, 0xff);    // 全ての割り込みを受け付けない
    io_out8(PIC1_IMR, 0xff);    // 全ての割り込みを受け付けない

    io_out8(PIC0_ICW1, 0x11);   // エッジトリガモード
    io_out8(PIC0_ICW2, 0x20);   // IRQ0-7はINT20-27で受け付ける
    io_out8(PIC0_ICW3, 1 << 2); // PIC1はIRQ2で接続
    io_out8(PIC0_ICW4, 0x01);   // ノンバッファモード

    io_out8(PIC1_ICW1, 0x11);   // エッジトリガモード
    io_out8(PIC1_ICW2, 0x28);   // IRQ8-17はINT28-2fで受け付ける
    io_out8(PIC1_ICW3, 2);      // PIC1はIRQ2で接続
    io_out8(PIC1_ICW4, 0x01);   // ノンバッファモード

    io_out8(PIC0_IMR, 0xfb);    // PIC1以外は全て禁止
    io_out8(PIC1_IMR, 0xff);    // 全ての割り込みを受け付けない
}

