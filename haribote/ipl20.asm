; haribote-ipl
; TAB=4

CYLS    EQU     20              ; 何シリンダディスクからOSを読み込むかを設定する定数

		ORG		0x7c00			; このプログラムがどこに読み込まれるのか

; 以下は標準的なFAT12ディスクのための記述

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; ブートセクタの名前を自由に書いてよい（8バイト）
		DW		512				; 1セクタの大きさ（512にしなければいけない）
		DB		1				; クラスタの大きさ（1セクタにしなければいけない）
		DW		1				; FATがどこから始まるか（普通は1セクタ目からにする）
		DB		2				; FATの個数（2にしなければいけない）
		DW		224				; ルートディレクトリ領域の大きさ（普通は224エントリにする）
		DW		2880			; このドライブの大きさ（2880セクタにしなければいけない）
		DB		0xf0			; メディアのタイプ（0xf0にしなければいけない）
		DW		9				; FAT領域の長さ（9セクタにしなければいけない）
		DW		18				; 1トラックにいくつのセクタがあるか（18にしなければいけない）
		DW		2				; ヘッドの数（2にしなければいけない）
		DD		0				; パーティションを使ってないのでここは必ず0
		DD		2880			; このドライブ大きさをもう一度書く
		DB		0,0,0x29		; よくわからないけどこの値にしておくといいらしい
		DD		0xffffffff		; たぶんボリュームシリアル番号
		DB		"HARIBOTEOS "	; ディスクの名前（11バイト）
		DB		"FAT12   "		; フォーマットの名前（8バイト）
		RESB	18				; とりあえず18バイトあけておく

; プログラム本体

entry:
		MOV		AX,0			; レジスタ初期化
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; ディスクを読む
		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; シリンダ0
		MOV		DH,0			; ヘッド0
		MOV		CL,2			; セクタ2
		MOV		BX, 18 * 2 * CYLS - 1 	; 読み込ませたいセクタ数
		CALL	readfast

; ディスクを読みこんだ
		MOV		BYTE[0x0ff0], CYLS
		JMP		0xc200

;　終了、CPU待機
fin:
		HLT						; 何かあるまでCPUを停止させる
		JMP		fin				; 無限ループ

; 読み込み成功メッセージがあるメモリのアドレスを記録
success:
        MOV     SI, successmsg
        JMP     putloop

; エラーメッセージがあるメモリのアドレスを記録
error:
		MOV		SI, errormsg

; 出力ループ
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; SIに1を足す
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 一文字表示ファンクション
		MOV		BX,15			; カラーコード
		INT		0x10			; ビデオBIOS呼び出し
		JMP		putloop

; 読み込み成功メッセージ
successmsg:
        DB      0x0a, 0x0a      ; 改行2つ
        DB      "load some sector success"
        DB      0x0a            ; 改行
        DB      0

; 読み込み失敗メッセージ
errormsg:
		DB		0x0a, 0x0a		; 改行を2つ
		DB		"load error"
		DB		0x0a			; 改行
		DB		0

; ディスク読み込み(高速版), 複数セクタを同時に読み込む
; ES : 読み込み番地, CH : シリンダ, DH : ヘッダ, CL : セクタ, RX : 読み込みセクタ数
readfast:
		MOV		AX, ES 			; <ESからセクタ数(AL)を計算>
		SHL		AX, 3			; AXを32で割ってAHに結果を代入したのと同じ動きをする AX -> AH | AL
		AND		AH, 0x7f 		; 128で割った余りを求める
		MOV		AL, 128 		; AL = 128 - AH => 一番近い64KBの境界まで何セクタあるか
		SUB		AL, AH 			; 1セクタ(512KB) * 128 - 1セクタ(512KB) - AH(端数)

		MOV		AH, BL			; ALの値を調節する
		CMP 	BH, 0 			; if(BH != 0){ AH = 18; }
		JE 		.skip1
		MOV		AH, 18
.skip1:
		CMP		AL, AH 			; if(AL > AH){ AL = AH; }
		JBE		.skip2
		MOV		AL, AH
.skip2:
		MOV		AH, 19
		SUB		AH, CL
		CMP 	AL, AH 			; if(AL > AH) { AL = AH; }
		JBE 	.skip3
		MOV		AL, AH
.skip3:
		PUSH	BX				; 失敗回数を数えるレジスタ
		MOV 	SI, 0

retry:
		MOV		AH, 0x02 		; ディスク読み込み命令のために設定
		MOV		BX, 0
		MOV		DL, 0x00
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH 	AX
		INT 	0x13			; ディスク読み出し(ALに設定したセクタ分読む)
		JNC		next			; エラーが起きなければnextへ
		ADD		SI, 1
		CMP		SI, 5 			; 5回連続で読み込みに失敗したら
		JAE		error
		MOV		AH, 0x00
		MOV		DL, 0x00
		INT		0x13			; ドライブリセット
		POP 	AX
		POP 	CX
		POP 	DX
		POP 	ES
		JMP 	retry
next:
		POP 	AX
		POP 	CX
		POP 	DX
		POP 	BX
		SHR		BX, 5 			; BXを512バイト単位にする(5ビット右シフト)
		MOV		AH, 0
		ADD		BX, AX
		SHL		BX, 5 			; BXを512バイト単位から32バイト単位へ(5ビット右シフト)
		MOV		ES, BX 			; ES += AL * 0x20
		POP 	BX
		SUB 	BX, AX
		JZ		.ret
		ADD    	CL, AL
		CMP 	CL, 18 			; if(CL <= 18){ goto readfast; }
		JBE		readfast
		MOV		CL, 1
		ADD 	DH, 1
		CMP 	DH, 2 	 		; if(DH < 2){ goto readfast; }
		JB 		readfast
		MOV 	DH, 0
		ADD 	CH, 1
		JMP 	readfast
.ret:
		RET


		RESB	0x7dfe-($-$$)-0x7c00		; 0x7dfeまでを0x00で埋める命令

		DB		0x55, 0xaa

