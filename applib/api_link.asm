[BITS 32]

    GLOBAL  api_putchar, api_putstr, api_end, api_gen_window, api_putstr_window, api_boxfill_window
	GLOBAL	api_init_memman, api_malloc, api_mfree
	GLOBAL	api_point_window, api_refresh_window, api_drawline_window, api_close_window
	GLOBAL	api_keyinput
	GLOBAL  api_alloc_timer, api_init_timer, api_set_timer, api_free_timer
	GLOBAL 	api_beep
	GLOBAL	api_fopen, api_fclose, api_fseek, api_fread, api_fsize
	GLOBAL	api_get_command, api_check_langmode

[SECTION .text]

api_putchar:	; void api_putchar(int a)
    MOV     EDX, 1
    MOV     AL, [ESP + 4]       ; 引数1
    INT     0x40                ; 1文字表示APIを呼ぶ
    RET

api_putstr:		; void _api_putstr(char *s)
	PUSH	EBX
	MOV		EDX, 2
	MOV		EBX, [ESP + 8]		; s
	INT		0x40
	POP		EBX
	RET

api_end:		; void api_end()
	MOV		EDX, 4
	INT 	0x40
	RET

api_gen_window:	; api_gen_window(char *buf, int width, int height, int back_color, char *title)
	PUSH	ESI
	PUSH	EDI
	PUSH	EBX
	MOV 	EDX, 5
	MOV		EBX, [ESP + 16]
	MOV		ESI, [ESP + 20]
	MOV		EDI, [ESP + 24]
	MOV		EAX, [ESP + 28]
	MOV		ECX, [ESP + 32]
	INT 	0x40
	POP		EBX
	POP		EDI
	POP		ESI
	RET

api_putstr_window:		; api_putstr_window(int win_id, int x, int y, int color, int str_len, char *str)
	PUSH	ESI
	PUSH	EDI
	PUSH	EBX
	PUSH	EBP
	MOV 	EDX, 6
	MOV		EBX, [ESP + 20]
	MOV		ESI, [ESP + 24]
	MOV		EDI, [ESP + 28]
	MOV		EAX, [ESP + 32]
	MOV		ECX, [ESP + 36]
	MOV		EBP, [ESP + 40]
	INT 	0x40
	POP 	EBP
	POP		EBX
	POP		EDI
	POP		ESI
	RET

api_boxfill_window:		; api_boxfill_window(int win_id, int x0, int y0, int x1, int y1, int color)
	PUSH	ESI
	PUSH	EDI
	PUSH	EBX
	PUSH	EBP
	MOV 	EDX, 7
	MOV		EBX, [ESP + 20]
	MOV		EAX, [ESP + 24]
	MOV		ECX, [ESP + 28]
	MOV		ESI, [ESP + 32]
	MOV		EDI, [ESP + 36]
	MOV		EBP, [ESP + 40]
	INT 	0x40
	POP 	EBP
	POP		EBX
	POP		EDI
	POP		ESI
	RET

api_init_memman:		; api_init_memnan()
	PUSH 	EBX
	MOV		EDX, 8
	MOV		EBX, [CS:0x0020]
	MOV		EAX, EBX
	ADD		EAX, 32*1204
	MOV		ECX, [CS:0x0000]
	SUB		ECX, EAX
	INT 	0x40
	POP 	EBX
	RET

api_malloc:				; api_malloc(int size)
	PUSH	EBX
	MOV		EDX, 9
	MOV		EBX, [CS:0x0020]
	MOV		ECX, [ESP + 8]
	INT 	0x40
	POP		EBX
	RET

api_mfree:				; api_mfree(char *addr, int size)
	PUSH	EBX
	MOV		EDX, 10
	MOV		EBX, [CS:0x0020]
	MOV		EAX, [ESP + 8]
	MOV		ECX, [ESP + 12]
	INT 	0x40
	POP		EBX
	RET

api_point_window:		; api_point_window(int win, int x, int y, int color)
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		EDX, 11
	MOV		EBX, [ESP + 16]
	MOV		ESI, [ESP + 20]
	MOV		EDI, [ESP + 24]
	MOV		EAX, [ESP + 28]
	INT  	0x40
	POP		EBX
	POP		ESI
	POP		EDI
	RET

api_refresh_window:		; api_refresh_window(int win, int x0, int y0, int x1, int y1)
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		EDX, 12
	MOV		EBX, [ESP + 16]
	MOV		EAX, [ESP + 20]
	MOV		ECX, [ESP + 24]
	MOV		ESI, [ESP + 28]
	MOV		EDI, [ESP + 32]
	INT 	0x40
	POP		EBX
	POP		ESI
	POP 	ESI
	RET

api_drawline_window:	; api_drawline_window(int win, int x0, int y0, int x1, int y1, int color)
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	PUSH	EBP
	MOV		EDX, 13
	MOV		EBX, [ESP + 20]
	MOV		EAX, [ESP + 24]
	MOV		ECX, [ESP + 28]
	MOV		ESI, [ESP + 32]
	MOV		EDI, [ESP + 36]
	MOV		EBP, [ESP + 40]
	INT 	0x40
	POP 	EBP
	POP		EBX
	POP		ESI
	POP 	ESI
	RET

api_close_window:		;api_close_window(int win)
	PUSH 	EBX
	MOV		EDX, 14
	MOV		EBX, [ESP + 8]
	INT 	0x40
	POP		EBX
	RET

api_keyinput:			; api_keyinput()
	MOV		EDX, 15
	MOV		EAX, [ESP + 4]
	INT 	0x40
	RET

api_alloc_timer:		; api_alloc_timer()
	MOV		EDX, 16
	INT 	0x40
	RET

api_init_timer:			; api_init_timer(int timer, int data)
	PUSH 	EBX
	MOV  	EDX, 17
	MOV		EBX, [ESP + 8]
	MOV		EAX, [ESP + 12]
	INT 	0x40
	POP 	EBX
	RET

api_set_timer:			; api_set_timer(int timer, int time)
	PUSH	EBX
	MOV		EDX, 18
	MOV		EBX, [ESP + 8]
	MOV		EAX, [ESP + 12]
	INT 	0x40
	POP		EBX
	RET

api_free_timer:			; api_free_timer(int timer)
	PUSH	EBX
	MOV		EDX, 19
	MOV		EBX, [ESP + 8]
	INT  	0x40
	POP 	EBX
	RET

api_beep:				; api_beep(int tone)
	MOV		EDX, 20
	MOV		EAX, [ESP + 4]
	INT 	0x40
	RET

api_fopen:				; api_fopen(char *name)
	PUSH 	EBX
	MOV		EDX, 21
	MOV		EBX, [ESP + 8]
	INT 	0x40
	POP 	EBX
	RET

api_fclose:				; api_fclose(int fhandle)
	MOV		EDX, 22
	MOV		EAX, [ESP + 4]
	INT 	0x40
	RET

api_fseek:				; api_fseek(int fhandle, int offset, int mode)
	PUSH	EBX
	MOV		EDX, 23
	MOV		EAX, [ESP + 8]
	MOV		EBX, [ESP + 12]
	MOV		ECX, [ESP + 16]
	INT 	0x40
	POP		EBX
	RET

api_fsize:				; api_fsize(int f_size, int mode)
	MOV		EDX, 24
	MOV		EAX, [ESP + 4]
	MOV		ECX, [ESP + 8]
	INT 	0x40
	RET

api_fread:				; api_fread(int fhandle, char *buf, int maxsize)
	PUSH	EBX
	MOV		EDX, 25
	MOV		EAX, [ESP + 8]
	MOV		EBX, [ESP + 12]
	MOV		ECX, [ESP + 16]
	INT 	0x40
	POP		EBX
	RET

api_get_command:		; api_get_command(char buf*, int maxsize)
	PUSH	EBX
	MOV		EDX, 26
	MOV		EBX, [ESP + 8]
	MOV		ECX, [ESP + 12]
	INT 	0x40
	POP 	EBX
	RET

api_check_langmode:
	MOV		EDX, 27
	INT 	0x40
	RET
