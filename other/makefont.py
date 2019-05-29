font_file = open("hankaku.txt", "r", encoding="utf-8")
font_lines = font_file.readlines()

c_file = open("hankaku.c", "w", encoding="utf-8")
c_file.write("char hankaku[4096] = {\n\n")

font_data = "\t"
for num, line in enumerate(font_lines):
    if num % 18 == 0:   # フォントのタイトル
        continue

    elif 1 <= num % 18 <= 16: # フォント本体(全部で16行)
        _01body = line.replace(".", "0") \
                      .replace("*", "1") \
                      .replace("\n", "")
        font_line_part = "0x" + format(int(_01body, 2), "x").zfill(2) + ", "
        font_data += font_line_part

    else:
        c_file.write(font_data[:-1] + "\n")
        font_data = "\t"

c_file.write("};")
c_file.close()
font_file.close()
