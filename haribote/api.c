#include "bootpack.h"

int *haribote_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax){
    struct TASK *task = task_now();
    int ds_base = task->ds_base;
    struct CONSOLE *console = task->console;
    struct SHEET_CTL *sheet_ctl = (struct SHEET_CTL *) *((int *) 0xfe4);
    struct FIFO32 *osfifo = (struct FIFO32 *) *((int *) 0xfec);

    // 値を返すために使う(PUSHADされた値を強制的に変える)
    // [レジスタ保存用PUSH][API用PUSH]の順でスタックに積まれている
    // [API用PUSH]の一番下がEAXなので、その次の値はレジスタの値を弄っているのと同じことになる
    // EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX : PUSHADではEAXからPUSHされる
    int *reg = &eax + 1;

    if(edx == 1){           // 1文字表示
        console_putchar(console, eax & 0xff, 1);
        return 0;
    }

    if(edx == 2){      // 1行表示
        console_putstr(console, (char *) ebx + ds_base);
        return 0;
    }

    if(edx == 3){      // 1行表示
        console_putstr_with_size(console, (char *) ebx + ds_base, ecx);
        return 0;
    }

    if(edx == 4){      // アプリ終了
        return &(task->tss.esp0);
    }

    if(edx == 5){      // ウィンドウ生成
        struct SHEET *sheet = sheet_alloc(sheet_ctl);
        sheet->flags |= 0x10;
        sheet_setbuf(sheet, (unsigned char *) ebx + ds_base, esi, edi, eax);
        make_window((unsigned char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
        sheet_slide(sheet, ((sheet_ctl->width - esi) / 2) & ~3, (sheet_ctl->height - edi) / 2);
        sheet_updown(sheet, sheet_ctl->top);
        sheet->task = task;
        reg[7] = (int) sheet;
        return 0;
    }

    if(edx == 6){      // ウィンドウに文字列を描画
        struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
        putstr8(sheet->buf, sheet->buf_width, esi, edi, eax, (char *)(ebp + ds_base));

        if((ebx & 1) == 0){
            sheet_refresh(sheet, esi, edi, ecx * 8, edi + 8);
        }
        return 0;
    }

    if(edx == 7){      // ウィンドウに四角形を描画
        struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
        boxfill8(sheet->buf, sheet->buf_width, ebp, eax, ecx, esi, edi);

        if((ebx & 1) == 0){
            sheet_refresh(sheet, eax, ecx, esi + 1, edi + 1);
        }
        return 0;
    }

    if(edx == 8){      // memman初期化
        memman_init((struct MEMMAN *) (ebx + ds_base));
        ecx &= 0xfffffff0;
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
        return 0;
    }

    if(edx == 9){      // malloc
        ecx = (ecx + 0x0f) & 0xfffffff0;
        reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
        return 0;
    }

    if(edx == 10){     // mfree
        ecx = (ecx + 0x0f) & 0xfffffff0;
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
        return 0;
    }

    if(edx == 11){      // ウィンドウに点を描画する
        struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
        sheet->buf[sheet->buf_width * edi + esi] = eax;

        if((ebx & 1) == 0){
            sheet_refresh(sheet, esi, edi, esi + 1, edi + 1);
        }
        return 0;
    }

    if(edx == 12){     // ウィンドウリフレッシュ
        struct SHEET *sheet = (struct SHEET *) ebx;
        sheet_refresh(sheet, eax, ecx, esi, edi);
        return 0;
    }

    if(edx == 13){      // ウィンドウに線を描画する
        struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
        drawline(sheet, eax, ecx, esi, edi, ebp);
        if((ebx & 1) == 0){
            if(eax > esi){
                int tmp = eax;
                eax = esi;
                esi = tmp;
            }
            if(ecx > edi){
                int tmp = ecx;
                ecx = edi;
                edi = tmp;
            }
            sheet_refresh(sheet, eax, ecx, esi + 1, edi + 1);
        }
        return 0;
    }

    if(edx == 14){      // ウィンドウを閉じる
        sheet_free((struct SHEET *) ebx);
        return 0;
    }

    if(edx == 15){      // キー入力
        while(1){
            // FIFO確認
            io_cli();
            if(fifo32_status(&task->fifo) == 0){
                if(eax != 0){
                    task_sleep(task);
                }else{
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
            }

            // キー入力を待っている間は他のことをしたり…
            int data = fifo32_get(&task->fifo);
            io_sti();
            if(data <= 1){  // タイマー
                timer_init(console->cursor_timer, &task->fifo, 1);
                timer_set(console->cursor_timer, 50);
            }else if(data == 3){    // カーソルON
                console->cursor_color = COL8_FFFFFF;
            }else if(data == 2){    // カーソルOFF
                console->cursor_color = -1;
            }else if(data == 4){
                timer_cancel(console->cursor_timer);
                io_cli();
                fifo32_put(osfifo, console->sheet - sheet_ctl->sheet_data + 2024);
                console->sheet = 0;
                io_sti();
            }else if(256 <= data){   // キー入力 or タイマ
                reg[7] = data - 256;
                return 0;
            }
        }
    }

    if(edx == 16){      // タイマ取得
        reg[7] = (int) timer_alloc();
        ((struct TIMER *) reg[7])->flags_auto_cancel = 1;
        return 0;
    }

    if(edx == 17){      // タイマ初期化
        timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
        return 0;
    }

    if(edx == 18){      // タイマ時間設定
        timer_set((struct TIMER *) ebx, eax);
        return 0;
    }

    if(edx == 19){		// タイマ解放
        timer_free((struct TIMER *) ebx);
        return 0;
    }

	if(edx == 20){      // BEEP音再生
        if(eax == 0){
            int beep_conf = io_in8(0x61);
            io_out8(0x61, beep_conf & 0x0d);
        }else{
            int beep_conf = 1193180000 / eax;
            io_out8(0x43, 0xb6);
            io_out8(0x42, beep_conf & 0xff);
            io_out8(0x42, beep_conf >> 8);
            beep_conf = io_in8(0x61);
            io_out8(0x61, (beep_conf | 0x03) & 0x0f);
        }
	}

    if(edx == 21){      // ファイルオープン
        // ファイルハンドルの空きを確認する
        int fh_idx;
        for(fh_idx = 0; fh_idx < 8; ++ fh_idx){
            if(task->fhandle[fh_idx].buf == 0){
                break;
            }
        }

        // ファイルを読み込も準備
        struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
        struct FILEINFO *finfo;
        struct FILEHANDLE *fhandle = &task->fhandle[fh_idx];
        int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
        read_fat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

        // ファイル検索->ファイル読み込み->ファイルハンドラを返す
        reg[7] = 0;
        if(fh_idx < 8){
            finfo = file_search((char *) ebx + ds_base, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
            if(finfo != 0){
                reg[7] = (int) fhandle;
                fhandle->buf = (char *) memman_alloc_4k(memman, finfo->size);
                fhandle->size = finfo->size;
                fhandle->pos = 0;
                load_file(finfo->clustno, finfo->size, fhandle->buf, fat, (char *) (ADR_DISKIMG + 0x003e00));
            }
        }

        memman_free_4k(memman, (int) fat, 4 * 2880);
        return 0;
    }

    if(edx == 22){      // ファイルクローズ
        struct FILEHANDLE *fhandle = (struct FILEHANDLE *) eax;
        struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
        memman_free_4k(memman, (int) fhandle->buf, fhandle->size);
        fhandle->buf = 0;
        return 0;
    }

    if(edx == 23){      // ファイルシーク
        struct FILEHANDLE *fhandle = (struct FILEHANDLE *) eax;
        if(ecx == 0){
            fhandle->pos = ebx;
        }else if(ecx == 1){
            fhandle->pos += ebx;
        }else if(ecx == 2){
            fhandle->pos = fhandle->size + ebx;
        }
        fhandle->pos = max(0, fhandle->pos);
        fhandle->pos = min(fhandle->pos, fhandle->size);
        return 0;
    }

    if(edx == 24){      // ファイルサイズ取得
        struct FILEHANDLE *fhandle = (struct FILEHANDLE *) eax;
        if(ecx == 0){
            reg[7] = fhandle->size;
        }else if(ecx == 1){
            reg[7] = fhandle->pos;
        }else if(ecx == 2){
            reg[7] = fhandle->pos - fhandle->size;
        }

        return 0;
    }

    if(edx == 25){      // ファイル読み込み
        struct FILEHANDLE *fhandle = (struct FILEHANDLE *) eax;
        int idx;
        for(idx = 0; idx < ecx && fhandle->pos != fhandle->size; ++ idx){
            *((char *) ebx + ds_base + idx) = fhandle->buf[fhandle->pos];
            ++ fhandle->pos;
        }
        reg[7] = idx;
        return 0;
    }

    if(edx == 26){      // コマンドライン取得
        int idx;
        for(idx = 0; ; ++ idx){
            *((char *) ebx + ds_base + idx) = task->command[idx];
            if(task->command[idx] == 0 || eax <= idx){
                break;
            }
        }
        reg[7] = idx;
        return 0;
    }

    if(edx == 27){
        reg[7] = task->langmode;
        return 0;
    }

    return 0;
}
