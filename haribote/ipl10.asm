; haribote-ipl
; TAB=4

CYLS    EQU     10              ; 何シリンダディスクからOSを読み込むかを設定する定数

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

readloop:
        MOV     SI, 0           ; 読み込み失敗回数カウンタ初期化

retry:
        MOV     AH, 0x02
        MOV     AL, 1           ; 1セクタだけ読み込む
        MOV     BX, 0
        MOV     DL, 0x00        ; USBはHDDとして認識される
        INT     0x13            ; ディスクBIOS呼び出し
        JNC     next            ; エラーがなければnextへ
        ADD     SI, 1           ; 失敗カウンタ+1
        CMP     SI, 5           ; 失敗回数が5なら…エラー表示
        JAE     error
        MOV     AH, 0x00
        MOV     DL, 0x00
        INT     0x13            ; システムリセット
        JMP     retry           ; もう一度読み込みに挑戦する

next:
        MOV     AX, ES
        ADD     AX, 0x0020      ; アドレスを0x200進める
        MOV     ES, AX          ; ADD ES, 0x200という命令がないのでAXレジスタを経由している
        ADD     CL, 1           ; 1セクタ進める
        CMP     CL, 18
        JBE     readloop        ; 読み込みセクタが18以下ならreadloopへ
        MOV     CL, 1
        ADD     DH, 1
        CMP     DH, 2
        JB      readloop        ; 読み込みヘッダが<2ならreadloopへ
        MOV     DH, 0
        ADD     CH, 1
        CMP     CH, CYLS
        JB      readloop        ; 読み込みシリンダが<CYLS(定数)ならreadloopへ

        MOV     [0x0ff0], CH
        JMP     0xC200          ; 別ファイルのOSを呼び出し

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

		RESB	0x7dfe-($-$$)-0x7c00		; 0x7dfeまでを0x00で埋める命令

		DB		0x55, 0xaa

