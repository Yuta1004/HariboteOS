#include "bootpack.h"

/*  ここからGDT, IDTの設定   */
void init_gdt_idt(void){
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) ADR_IDT;

    // GDT初期化
    for(int i = 0; i < LIMIT_GDT / 8; i++){
        set_segmdesc(gdt + i, 0, 0, 0);
    }

    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);  // メモリ全体のセグメント
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);  // bootpack.hrbのためのセグメント
    load_gdtr(LIMIT_GDT, ADR_GDT);

    // IDT初期化
    for(int i = 0; i < LIMIT_IDT / 8; i++){
        set_gatedesc(idt + i, 0, 0, 0);
    }

    // 割り込みが発生したらasm_inthander_**を呼び出すようにIDTを設定している
    set_gatedesc(idt + 0x00, (int) asm_inthandler00, 2 << 3, AR_INTGATE32);
    set_gatedesc(idt + 0x0c, (int) asm_inthandler0c, 2 << 3, AR_INTGATE32);
    set_gatedesc(idt + 0x0d, (int) asm_inthandler0d, 2 << 3, AR_INTGATE32);
    set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 << 3, AR_INTGATE32);
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 << 3, AR_INTGATE32);
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2C, 2 << 3, AR_INTGATE32);

    // APIをIDTに登録してどこからでも呼べるようにする
    set_gatedesc(idt + 0x40, (int) asm_haribote_api, 2 << 3, AR_INTGATE32 + 0x60);

    // IDT読み込み
    load_idtr(LIMIT_IDT, ADR_IDT);
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
    // limitが20ビットを超えるようならGビットを1にする
    if(limit > 0xfffff){
        ar |= 0x8000;  // G_bit = 1
        limit /= 0x1000;
    }

    /*
    * 00000000000000000000000000000000
    *                 ----------------    // base_low : base & 0xffff
    *         --------                    // base_mid : (base >> 16) & 0xff
    * --------                            // base_high: (base >> 24) & 0xff
    */

    /*
     * 000000000000000000000000
     *         ----------------     // limit_low : limit & 0xffff
     *     ----                     // limit_high: ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0)
     *                              //  -> limit_highにはlimitの上位4ビットと拡張アクセス権が4ビットずつ置かれる
     */

    sd->limit_low = limit & 0xffff;
    sd->base_low = base & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high = (base >> 24) & 0xff;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar){
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
}
