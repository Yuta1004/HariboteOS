; nasmfunc

[BITS 32]               ; 32ビットモード用に機械語を生成する

; オブジェクトファイルのための情報
        GLOBAL  io_hlt, io_cli, io_sti, io_stihlt
        GLOBAL  io_in8, io_in16, io_in32, io_out8, io_out16, io_out32
        GLOBAL  io_load_eflags, io_store_eflags
        GLOBAL  load_gdtr, load_idtr
        GLOBAL  asm_inthandler20, asm_inthandler21, asm_inthandler2C, asm_inthandler0d, asm_inthandler0c, asm_inthandler00
        GLOBAL  load_cr0, store_cr0
        GLOBAL  load_tr, far_jmp, far_call
        GLOBAL  memtest_sub
        GLOBAL  asm_haribote_api
        EXTERN  inthandler20, inthandler21, inthandler2C, inthandler0d, inthandler0c, inthandler00
        EXTERN  haribote_api
        EXTERN  start_app, end_app


[SECTION .text]

io_hlt:         ; void io_hlt(void)
        HLT
        RET

io_cli:         ; void io_cli(void)
        CLI
        RET

io_sti:         ; void io_str(void)
        STI
        RET

io_stihlt:      ; void io_stihlt(void)
        STI
        HLT
        RET

io_in8:         ; int io_in8(int port)
        MOV     EDX, [ESP+4]
        MOV     EAX, 0
        IN      AL, DX
        RET

io_in16:        ; int io_in16(int port)
        MOV     EDX, [ESP+4]
        MOV     EAX, 0
        IN      AX, DX
        RET

io_in32:        ; int io_in32(int port)
        MOV     EDX, [ESP+4]
        IN      EAX, DX
        RET

io_out8:        ; void io_out8(int port, int data)
        MOV     EDX, [ESP+4]
        MOV     AL, [ESP+8]
        OUT     DX, AL
        RET

io_out16:       ; void io_out16(int port, int data)
        MOV     EDX, [ESP+4]
        MOV     EAX, [ESP+8]
        OUT     DX, AX
        RET

io_out32:       ; void io_out32(int port, int data)
        MOV     EDX, [ESP+4]
        MOV     EAX, [ESP+8]
        OUT     DX, EAX

io_load_eflags:     ; int io_load_eflags(void)
        PUSHFD
        POP     EAX
        RET

io_store_eflags:    ; int io_store_eflags(int eflags)
        MOV     EAX, [ESP+4]
        PUSH    EAX
        POPFD
        RET

load_gdtr:      ; void load_gdtr(int limit, int addr)
        MOV     AX, [ESP+4]
        MOV     [ESP+6], AX
        LGDT    [ESP+6]
        RET

load_idtr:      ; void load_idtr(int limit, int addr)
        MOV     AX, [ESP+4]
        MOV     [ESP+6], AX
        LIDT    [ESP+6]
        RET

; 割り込み
; レジスタの値を全部退避させて割り込みを受けている
asm_inthandler00:
		STI
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX, ESP
        PUSH    EAX
        MOV     AX, SS
        MOV     DS, AX
        MOV     ES, AX
        CALL    inthandler00
        CMP	EAX, 0
        JNE 	end_app
        POP	EAX
        POPAD
        POP     DS
        POP     ES
        ADD     ESP, 4              ; 0x00の割り込みではこれが必要
        IRETD

asm_inthandler0c:
		STI
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX, ESP
        PUSH    EAX
        MOV     AX, SS
        MOV     DS, AX
        MOV     ES, AX
        CALL    inthandler0c
        CMP	EAX, 0
        JNE 	end_app
        POP	EAX
        POPAD
        POP     DS
        POP     ES
        ADD     ESP, 4              ; 0x0cの割り込みではこれが必要
        IRETD

asm_inthandler0d:
		STI
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX, ESP
        PUSH    EAX
        MOV     AX, SS
        MOV     DS, AX
        MOV     ES, AX
        CALL    inthandler0d
        CMP     EAX, 0
        JNE     end_app
        POP     EAX
        POPAD
        POP     DS
        POP     ES
        ADD     ESP, 4              ; 0x0dの割り込みではこれが必要
        IRETD

asm_inthandler20:
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX, ESP
        PUSH    EAX
        MOV     AX, SS
        MOV     DS, AX
        MOV     ES, AX
        CALL    inthandler20
        POP     EAX
        POPAD
        POP     DS
        POP     ES
        IRETD

asm_inthandler21:
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX, ESP
        PUSH    EAX
        MOV     AX, SS
        MOV     DS, AX
        MOV     ES, AX
        CALL    inthandler21
        POP		EAX
        POPAD
        POP     DS
        POP     ES
        IRETD

asm_inthandler2C:
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX, ESP
        PUSH    EAX
        MOV     AX, SS
        MOV     DS, AX
        MOV     ES, AX
        CALL    inthandler2C
        POP		EAX
        POPAD
        POP     DS
        POP     ES
        IRETD

; メモリチェック
load_cr0:       ; int load_cr0(void)
        MOV     EAX, CR0
        RET

store_cr0:      ; void store_cr0(int cr0)
        MOV     EAX, [ESP+4]
        MOV     CR0, EAX
        RET

memtest_sub:    ; unsigned int memtest_sub(unsigned int start, unsigned int end)
        PUSH    EDI
        PUSH    ESI
        PUSH    EBX
        MOV     ESI, 0xaa55aa55     ; pat0 = 0xaa55aa55
        MOV     EDI, 0x55aa55aa     ; pat1 = 0x55aa55aa
        MOV     EAX, [ESP + 12 + 4] ; idx = start
mts_loop:
        MOV     EBX, EAX
        ADD     EBX, 0xffc          ; p = i + 0xffc
        MOV     EDX, [EBX]          ; old = *p
        MOV     [EBX], ESI          ; *p = pat0
        XOR     DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff
        CMP     EDI, [EBX]          ; if(pat1 != *p) goto fin
        JNE     mts_fin
        XOR     DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff
        CMP     ESI, [EBX]          ; if(pat0 != *p) goto fin
        JNE     mts_fin
        MOV     [EBX], EDX          ; *p = old
        ADD     EAX, 0x1000         ; idx += 0x1000
        CMP     EAX, [ESP + 12 + 8] ; if(idx <= goal) goto mts_loop
        JBE     mts_loop
        POP     EBX
        POP     ESI
        POP     EDI
        RET
mts_fin:
        MOV     [EBX], EDX          ; *p = old
        POP     EBX
        POP     ESI
        POP     EDI
        RET

load_tr:        ; void load_tr(int tr)
        LTR     [ESP+4]
        RET

far_jmp:         ; void far_jmp(int eip, int cs)
        JMP     FAR [ESP+4]
        RET

far_call:        ; void far_call(int eip, int cs
        CALL    FAR [ESP + 4]
        RET

; API
asm_haribote_api:
		STI
        PUSH    DS
        PUSH    ES
        PUSHAD                      ; 値保存用
	PUSHAD                      ; APIに渡すためのPUSH
        MOV     AX, SS
        MOV     ES, AX              ; OS用のセグメント情報をESとDSに入れる
        MOV     DS, AX

	CALL    haribote_api

        CMP     EAX, 0
        JNE     end_app				; アプリ強制終了
        ADD     ESP, 32				; スタック領域を狭める
        POPAD
        POP     ES
        POP     DS
        IRETD

end_app:
        MOV     ESP, [EAX]                      ; apiからTSS.esp0の番地が返ってくる
        MOV     DWORD [EAX + 4], 0              ; ESPを0にする
        POPAD
        RET

; アプリ実行のための設定をする関数
start_app:      ; start_app(int eip, int cs, int esp, int ds)
        PUSHAD                      ; 32ビットレジスタを全部保存
        MOV     EAX, [ESP + 36]     ; アプリ用のEIP
        MOV     ECX, [ESP + 40]     ; アプリ用のCS
        MOV     EDX, [ESP + 44]     ; アプリ用のESP
        MOV     EBX, [ESP + 48]     ; アプリ用のDS
        MOV     EBP, [ESP + 52]         ; tss.ep0の番地
        MOV     [EBP], ESP              ; OSのESPをTSSに登録する
        MOV     [EBP + 4], SS           ; SSをTSSに登録する
        MOV     ES, BX                  ; セグメントレジスタには全て同じ値を入れておくのがスタンダード…とのこと
        MOV     DS, BX
        MOV     FS, BX
        MOV     GS, BX
        ; アプリをRETFで呼び出すためにスタック調整 -> RETF
        OR      ECX, 3
        OR      EBX, 3
        PUSH    EBX
        PUSH    EDX
        PUSH    ECX
        PUSH    EAX
        RETF

