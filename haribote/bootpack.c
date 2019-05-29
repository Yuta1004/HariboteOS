#include "bootpack.h"

void HariMain(void){
    // 起動情報受け取り
    struct BOOTINFO *binfo = (struct BOOTINFO*) 0x0ff0;

    // GDT,IDT初期化 → 割り込み許可
    init_gdt_idt();
    init_pic();
    io_sti();   // PICやIDTの設定が終わったので割り込みを許可する

    // タイマ割り込み設定
    init_pit();

    // IMRを変更してPIT, PIC1, キーボード, マウスからの割り込みを受け付ける
    io_out8(PIC0_IMR, 0xf8);
	io_out8(PIC1_IMR, 0xef);

    // メモリチェック & 解放
    extern unsigned int mem_size;
    mem_size = memtest(0x00400000, 0xbfffffff);
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, mem_size - 0x00400000);

    // 日本語フォントファイル(jpn16v00.bin)を読み込む
    extern char hankaku[4096];
    unsigned char *japanese;
    struct FILEINFO *finfo = file_search("jpn_tek.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    read_fat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

    // フォント設定
    if(finfo != 0){
        int size = finfo->size;
        japanese = load_file_from_tek(finfo->clustno, &size, fat);
    }else{
        japanese = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
        for(int idx = 0; idx < 16 * 256; ++ idx){   // 半角部分のみを既存のフォントで置き換える
            japanese[idx] = hankaku[idx];
        }
        for(int idx = 16 * 256; idx < 16 * 256 + 32 * 94 * 47; ++ idx){     // 日本語部分は0xffで埋める
            japanese[idx] = 0xff;
        }
    }
    *((int *) 0x0fe8) = (int) japanese;
    memman_free_4k(memman, (int) fat, 4 * 2880);

    // FIFO32初期化
    struct FIFO32 osfifo, keycmd_fifo;
    int osfifobuf[128], keycmd_fifo_buf[128];
    fifo32_init(&osfifo, 128, osfifobuf, 0);
    fifo32_init(&keycmd_fifo, 128, keycmd_fifo_buf, 0);

    // sprintf用の変数(使いまわしていく)
    char s[40];

    // シート初期化
    struct SHEET_CTL *sheet_ctl = sheet_ctl_init(memman, binfo->vram, binfo->width, binfo->height);
    struct SHEET *sheet_back = sheet_alloc(sheet_ctl);
    struct SHEET *sheet_mouse = sheet_alloc(sheet_ctl);
    unsigned char *buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->width * binfo->height);
    unsigned char *buf_console[2];
    unsigned char buf_mouse[256];
    sheet_setbuf(sheet_back, buf_back, binfo->width, binfo->height, -1);
    sheet_setbuf(sheet_mouse, buf_mouse, 16, 16, 99);
    *((int *) 0xfe4) = (int) sheet_ctl;
    *((int *) 0xfec) = (int) &osfifo;

    // メイン画面初期化
    init_palette();
    init_screen(buf_back, binfo->width, binfo->height);

    // マウス描画用
    struct MOUSE_DEC mdec;
    int mouse_x = (binfo->width - 16) / 2;
    int mouse_y = (binfo->height - 28 - 16) / 2;
    int mouse_draw_x = -1, mouse_draw_y = 0;
    init_mouse_cursor8(buf_mouse, 99);

    // キーボードコントローラ初期化 -> マウス有効化
    init_keyboard(&osfifo, 256);
    enable_mouse(&osfifo, 512, &mdec);

    // タスク管理初期化
    struct TASK *task_main;
    task_main = task_init(memman);
    task_run(task_main, 1, 0);
    osfifo.task = task_main;
    task_main->langmode = 0;

    // 入力モードを切り替えるための変数
    struct SHEET *key_to_window = 0;

    // シート描画位置指定
    sheet_slide(sheet_back, 0, 0);
    sheet_slide(sheet_mouse, mouse_x, mouse_y);
    sheet_slide(key_to_window, 100, 160);

    // シート描画順位指定
    sheet_updown(sheet_back, 0);
    sheet_updown(key_to_window, 1);
    sheet_updown(sheet_mouse, 2);

    // キーの状態
    int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

    // ウィンドウ移動用変数
    int win_move_x = -1, win_move_y = -1, win_move_vx = -1;
    int win_draw_x = 0x7fffffff, win_draw_y = 0;
    struct SHEET *select_sheet;

    // メインループ
    while(1){
        /*  キーボードコントローラーへデータを送信する */
        if(fifo32_status(&keycmd_fifo) != 0 && keycmd_wait < 0){
            keycmd_wait = fifo32_get(&keycmd_fifo);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }

        io_cli();

        // メモリ情報
        // msprintf(s, "MEMORY FREE : %d KB", (memman_total(memman) / 1024));
        // putstr8_ref(sheet_back, 0, 16, COL8_000000, COL8_FFFFFF, s, 25);

        // FIFOバッファが空だった
        if(fifo32_status(&osfifo) == 0){
            if(mouse_draw_x >= 0){
                io_sti();
                sheet_slide(sheet_mouse, mouse_draw_x, mouse_draw_y);
                mouse_draw_x = -1;
            }else if(win_draw_x != 0x7fffffff){
                io_sti();
                sheet_slide(key_to_window, win_draw_x, win_draw_y);
                win_draw_x = 0x7fffffff;
            }else{
                task_sleep(task_main);
                io_sti();
            }
            continue;
        }

        // FIFOバッファからデータを取得
        int data = fifo32_get(&osfifo);
        io_sti();

        // 入力モードになっているウィンドウが閉じられた
        if(key_to_window != 0 && key_to_window->flags == 0){
            if(sheet_ctl->top == 1){    // 入力対象のウィンドウが存在しない
                key_to_window = 0;
            }else{
                key_to_window = sheet_ctl->sheets[sheet_ctl->top - 1];
                key_window_on(key_to_window);
            }
        }

        /*  キーボード  */
        if(256 <= data && data <= 511){
            extern char keytable[2][0x80];

            // 入力表示
            // msprintf(s, "%d, %x", data - 256, data - 256);
            // putstr8_ref(sheet_back, 0, 0, COL8_000000, COL8_FFFFFF, s, 10);

            // コンソール上のアプリケーションを強制終了する(backspace + shift)
            if(data == 256 + 0x0e && key_shift){
                struct TASK *task = key_to_window->task;
                if(task != 0 && task->tss.ss0 != 0){
                    // 強制終了割り込み表示
                    struct CONSOLE *console = task->console;
                    console_putstr(console, "\nKeyboard Interrupt\n");

                    // end_appに飛ぶようにする
                    io_cli();
                    task->tss.eax = (int) &(task->tss.esp0);
                    task->tss.eip = (int) end_app;
                    io_sti();
                    task_run(task, -1, 0);
                    continue;
                }
            }

            // 一番下のウィンドウを一番上に引っ張り上げる (ctrl + shift)
            if(data == 256 + 0x1d && key_shift){
                sheet_updown(sheet_ctl->sheets[1], sheet_ctl->top - 1);
                continue;
            }

            // コンソールを生成する
            if(data == 256 + 0x0f && key_shift){
                if(key_to_window != 0){
                    key_window_off(key_to_window);
                }
                key_to_window = open_console(sheet_ctl, mem_size);
                key_window_on(key_to_window);
                sheet_slide(key_to_window, 300, 240);
                sheet_updown(key_to_window, sheet_ctl->top);
            }

            // シフトキー
            if(data == 256 + 0x2a) key_shift |= 1;
            if(data == 256 + 0x36) key_shift |= 2;
            if(data == 256 + 0xaa) key_shift &= ~1;
            if(data == 256 + 0xb6) key_shift &= ~2;
            if(data == 256 + 0x2a || data == 256 + 0x36 || data == 256 + 0xaa || data == 256 + 0xb6){
                continue;
            }

            // キー情報パース
            char keydata = (data < 256 + 0x80) * keytable[key_shift > 0][data - 256];
            if('A' <= keydata && keydata <= 'Z'){
                if((key_shift == 0 && (key_leds % 4 == 0)) || (key_shift > 0 && (key_leds % 4 != 0))){
                    keydata += 0x20;
                }
            }

            // 通常キー
            if(keydata != 0 && key_to_window != 0){
                fifo32_put(&key_to_window->task->fifo, keydata + 256);
            }

            // BackSpace
            if(data == 256 + 0x0e && key_to_window != 0){
                fifo32_put(&key_to_window->task->fifo, 8 + 256);
            }

            // Enter
            if(data == 256 + 0x1c && key_to_window != 0){
                fifo32_put(&key_to_window->task->fifo, 10 + 256);
            }

            // Tab(入力ウィンドウ切り替え)
            if(data == 256 + 0x0f && key_to_window != 0){
                key_window_off(key_to_window);
                int next_sheet_idx = key_to_window->layer_height - 1;
                if(next_sheet_idx == 0){
                    next_sheet_idx = sheet_ctl->top - 1;
                }
                key_to_window = sheet_ctl->sheets[next_sheet_idx];
                key_window_on(key_to_window);
            }

            // Lock系キー制御
            if(data == 256 + 0x3a) key_leds ^= 4;       // CapsLock
            if(data == 256 + 0x45) key_leds ^= 2;       // NumLock
            if(data == 256 + 0x46) key_leds ^= 1;       // ScrollLock
            if(data == 256 + 0xfa) keycmd_wait = -1;    // キーボードに無事データを送れた
            if(data == 256 + 0x3a || data == 256 + 0x45 || data == 256 + 0x45){     // FIFOに値追加
                fifo32_put(&keycmd_fifo, KEYCMD_LED);
                fifo32_put(&keycmd_fifo, key_leds);
            }
            if(data == 256 + 0xfe){                     // キーボードへのデータ送信が失敗した
                wait_KBC_sendready();
                io_out8(PORT_KEYDAT, keycmd_wait);
            }

            continue;
        }

        /*  マウス  */
        if(512 <= data && data <= 767){
            // マウスデータを3バイト受信できたら描画
            if(mouse_decode(&mdec, data - 512) != 0){
                // マウス描画準備
                mouse_x += mdec.x;
                mouse_y += mdec.y;
                mouse_draw_x = mouse_x;
                mouse_draw_y = mouse_y;
                if(mouse_x < 0) mouse_x = 0;
                if(mouse_y < 0) mouse_y = 0;
                if(mouse_x > binfo->width - 1) mouse_x = binfo->width - 1;
                if(mouse_y > binfo->height - 1) mouse_y = binfo->height - 1;

                // 左クリック
                if((mdec.btn & 0x01) != 0){
                    if(win_move_x < 0){
                        // 上からウィンドウを見ていく…
                        for(int sheet_idx = sheet_ctl->top - 1; sheet_idx > 0; -- sheet_idx){
                            struct SHEET *sheet = sheet_ctl->sheets[sheet_idx];
                            select_sheet = sheet;
                            int x = mouse_x - sheet->vram_x;
                            int y = mouse_y - sheet->vram_y;

                            // ウィンドウをクリックした
                            if(0 <= x && x < sheet->buf_width && 0 <= y && y < sheet->buf_height){
                                if(sheet->buf[sheet->buf_width * y + x] != sheet->col_inv){
                                    // ウィンドウを上に持ち上げる -> タイトルバークリックで移動モードに
                                    sheet_updown(sheet, sheet_ctl->top - 1);
                                    if(3 <= x && x < sheet->buf_width - 3 && 3 <= y && y < 21){
                                        win_move_x = mouse_x;
                                        win_move_y = mouse_y;
                                        win_move_vx = sheet->vram_x;
                                        win_draw_y = sheet->vram_y;
                                    }

                                    // 入力モードにする
                                    key_window_off(key_to_window);
                                    key_to_window = sheet;
                                    key_window_on(key_to_window);

                                    // ウィンドウを閉じる(=強制終了)
                                    if(sheet->buf_width - 21 <= x && x < sheet->buf_width - 5 && 5 <= y && y < 19){
                                        if((sheet->flags & 0x10) != 0){
                                            struct TASK *task = sheet->task;
                                            struct CONSOLE *console = task->console;
                                            console_putstr(console, "\nExit\n");
                                            io_cli();
                                            task->tss.eax = (int) &(task->tss.esp0);
                                            task->tss.eip = (int) end_app;
                                            io_sti();
                                            task_run(task, -1, 0);
                                        }else{
                                            struct TASK *task = sheet->task;
                                            io_cli();
                                            fifo32_put(&task->fifo, 4);
                                            io_sti();
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }else{
                        // ウィンドウ移動モード
                        int moved_x = mouse_x - win_move_x;
                        int moved_y = mouse_y - win_move_y;
                        win_draw_x = (win_move_vx + moved_x + 2) & ~3;
                        win_draw_y += moved_y;
                        win_move_y = mouse_y;
                    }
                }else{
                    // 移動モード終了
                    win_move_x = -1;
                    if(win_draw_x != 0x7fffffff){
                        sheet_slide(key_to_window, win_draw_x, win_draw_y);
                        win_draw_x = 0x7fffffff;
                    }
                }
            }
            continue;
        }

        /* コンソールタスクを殺す */
        if(768 <= data && data <= 1023){
            close_console(sheet_ctl->sheet_data + (data - 768));
            continue;
        }
        if(1024 <= data && data <= 2023){
            kill_console_task(task_ctl->tasks + (data - 1024));
            continue;
        }
        if(2024 <= data && data <= 2279){
            struct SHEET *sheet = sheet_ctl->sheet_data + (data - 2024);
            memman_free_4k(memman, (int) sheet->buf, 400 * 300);
            sheet_free(sheet);
        }
    }
}
