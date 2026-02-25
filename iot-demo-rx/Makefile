# ==============================================================================
# SAMDEMO - GR-SAKURA (RX63N) ファームウェア Makefile
# ==============================================================================
# 使い方:
#   make build            ビルドのみ
#   make flash PORT=COM3  ビルド後に書き込み
#   make monitor PORT=COM3 シリアルモニタ起動
#   make clean            中間ファイルを削除
# ==============================================================================

# --- ツールチェーン ---
CC       = rx-elf-gcc
OBJCOPY  = rx-elf-objcopy
OBJDUMP  = rx-elf-objdump
SIZE     = rx-elf-size

# --- コンパイルフラグ ---
CFLAGS   = -mcpu=rx600 \
           -O2 \
           -Wall \
           -Wextra \
           -ffunction-sections \
           -fdata-sections \
           -I./src

# --- リンクフラグ ---
LDFLAGS  = -T ./linker/rx63n.ld \
           -Wl,--gc-sections \
           -Wl,-Map=$(TARGET).map \
           -nostartfiles

# --- 書き込みポート（環境変数 PORT または make 引数で上書き可能）---
PORT    ?= COM3

# --- ターゲット名 ---
TARGET   = firmware

# --- ソースファイル ---
SRCS     = src/main.c \
           startup/startup.c

# --- オブジェクトファイル（src/ 直下のみ、ディレクトリ構造を保持しない）---
OBJS     = $(SRCS:.c=.o)

# ==============================================================================
# ターゲット定義
# ==============================================================================

.PHONY: all build flash monitor clean size disasm

all: build

# --- ビルド ---
build: $(TARGET).mot
	@echo ""
	@echo "=== ビルド完了: $(TARGET).mot ==="
	$(SIZE) $(TARGET).elf

$(TARGET).mot: $(TARGET).elf
	$(OBJCOPY) -O srec $< $@
	@echo ">>> .mot ファイル生成: $@"

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@echo ">>> ELF 生成: $@"

%.o: %.c
	@echo ">>> コンパイル: $<"
	$(CC) $(CFLAGS) -c -o $@ $<

# --- 書き込み（make flash PORT=COM3）---
flash: build
	@echo ""
	@echo "=== GR-SAKURA への書き込み開始 (PORT=$(PORT)) ==="
	@echo "注意: GR-SAKURA が $(PORT) に接続されていることを確認してください"
	rfp-cli -d rx -t com -p $(PORT) -a $(TARGET).mot
	@echo "=== 書き込み完了 ==="

# --- シリアルモニタ ---
monitor:
	@echo "=== シリアルモニタ起動 (PORT=$(PORT), 115200bps) ==="
	python ../tools/monitor.py $(PORT) 115200

# --- サイズ表示 ---
size: $(TARGET).elf
	$(SIZE) -A $<

# --- 逆アセンブル（デバッグ用）---
disasm: $(TARGET).elf
	$(OBJDUMP) -d $< > $(TARGET).asm
	@echo ">>> 逆アセンブル出力: $(TARGET).asm"

# --- クリーン ---
clean:
	rm -f src/*.o startup/*.o
	rm -f $(TARGET).elf $(TARGET).mot $(TARGET).map $(TARGET).asm
	@echo "=== クリーン完了 ==="

# --- 依存関係（ヘッダ変更時の再ビルド）---
-include $(OBJS:.o=.d)
