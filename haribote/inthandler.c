#include "bootpack.h"

// ゼロ除算例外
int *inthandler00(int *esp){
    struct TASK *task = task_now();
    struct CONSOLE *console = task->console;
    console_putstr(console, "\nINT 00 : Zero Division Exception.\n");
    return &(task->tss.esp0);
}

// スタック例外
int *inthandler0c(int *esp){
    struct TASK *task = task_now();
    struct CONSOLE *console = task->console;
    char str[30];
    msprintf(str, "\tEIP = %x\n", esp[11]);
    console_putstr(console, "\nINT 0C : Stack Exception.\n");
    console_putstr(console, str);
    return &(task->tss.esp0);
}

// セグメント外の参照例外
int *inthandler0d(int *esp){
    struct TASK *task = task_now();
    struct CONSOLE *console = task->console;
    console_putstr(console, "\nINT 0D : General Protected Exception.\n");
    return &(task->tss.esp0);
}

// タイマ割り込みを受ける(IRQ0)
void inthandler20(int *esp){
    io_out8(PIC0_OCW2, 0x60);   // 受信通知
    ++ timer_ctl.count;

    // 監視対象の時刻を過ぎたか
    if(timer_ctl.next > timer_ctl.count) return;

    struct TIMER *timer = timer_ctl.timer_lead;
    int cnt = 0;
    char taskswitch_flag = 0;

    // タイマーチェック
    timer_ctl.next = 0xffffffff;
    while(1){
        // まだ時間じゃない…
        if(timer->timeout > timer_ctl.count){
            break;
        }

        // タイムアウト (タスク切り替えタイマの判定もする)
        timer->flags = TIMER_FLAG_ALLOC;
        if(timer == task_timer){
            taskswitch_flag = 1;
        }else{
            fifo32_put(timer->fifo, timer->data);
        }
        timer = timer->next_timer;
    }

    // 後処理
    timer_ctl.timer_lead = timer;
    timer_ctl.next = timer->timeout;

    // タスクスイッチタイマがタイムアウトしてたら
    if(taskswitch_flag == 1){
        task_switch();
    }
}

// キーボード(IRQ1)からの割り込み
void inthandler21(int *esp){
    // IRQ1受付完了をPICに通知
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61);
    data = io_in8(PORT_KEYDAT);

    // FIFOバッファに書き込み
    fifo32_put(keyinfo, data + key_base);
}

// マウス(IRQ12)からの割り込み
void inthandler2C(int *esp){
    // IRQ12, IRQ2受付完了をPICに通知
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64);
    io_out8(PIC0_OCW2, 0x62);
    data = io_in8(PORT_KEYDAT);

    // FIFOバッファの書き込み
    fifo32_put(mouseinfo, data + mouse_base);
}

