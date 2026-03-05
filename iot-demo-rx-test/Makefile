# ==============================================================================
# GR-SAKURA (RX63N) FreeRTOS ファームウェア Makefile
# ==============================================================================
# 使い方:
#   make build            ビルドのみ
#   make flash            ビルド後に書き込み（ユーザー確認必須）
#   make clean            中間ファイルを削除
#   make disasm           逆アセンブル（デバッグ用）
# ==============================================================================

# --- ツールチェーンパス ---
GCC_DIR  = C:/Users/kouse/AppData/Roaming/GCC for Renesas RX 8.3.0.202411-GNURX-ELF/rx-elf/rx-elf/bin
RFP_DIR  = C:/Program Files (x86)/Renesas Electronics/Programming Tools/Renesas Flash Programmer V3.22

# --- ツールチェーン ---
CC       = "$(GCC_DIR)/rx-elf-gcc"
OBJCOPY  = "$(GCC_DIR)/rx-elf-objcopy"
OBJDUMP  = "$(GCC_DIR)/rx-elf-objdump"
SIZE     = "$(GCC_DIR)/rx-elf-size"
RFP_CLI  = "$(RFP_DIR)/rfp-cli"

# --- ターゲット名 ---
TARGET   = firmware

# --- インクルードパス ---
INCLUDES = -I./generate \
           -I./src \
           -I./freertos \
           -I./freertos/portable/GCC/RX600

# --- コンパイルフラグ ---
CFLAGS   = -mcpu=rx600 \
           -O2 \
           -Wall \
           -Wextra \
           -ffunction-sections \
           -fdata-sections \
           $(INCLUDES)

# --- リンクフラグ ---
LDFLAGS  = -T ./linker/rx63n_freertos.ld \
           -Wl,-Map=$(TARGET).map \
           -Wl,--gc-sections \
           -nostartfiles

# --- アプリケーションソース ---
APP_SRCS = src/main.c \
           src/sci2_uart.c \
           src/json_parser.c \
           src/json_builder.c \
           src/pid_ctrl.c \
           src/uart_task.c \
           src/pid_task.c \
           src/anomaly_task.c \
           src/wdt_task.c \
           src/status_task.c

# --- 生成コード ---
GEN_SRCS = generate/hwinit.c \
           generate/vects.c \
           generate/inthandler.c

# --- FreeRTOS カーネル ---
RTOS_SRCS = freertos/tasks.c \
            freertos/queue.c \
            freertos/list.c \
            freertos/timers.c \
            freertos/portable/GCC/RX600/port.c \
            freertos/portable/MemMang/heap_4.c

# --- 全ソース ---
SRCS     = $(APP_SRCS) $(GEN_SRCS) $(RTOS_SRCS)

# --- オブジェクトファイル ---
OBJS     = $(SRCS:.c=.o) generate/start.o

# ==============================================================================
# ターゲット定義
# ==============================================================================

.PHONY: all build flash clean size disasm

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

# C ソースのコンパイル
%.o: %.c
	@echo ">>> コンパイル: $<"
	$(CC) $(CFLAGS) -c -o $@ $<

# アセンブラ: generate/start.S → generate/start.o
generate/start.o: generate/start.S
	@echo ">>> アセンブル: $<"
	$(CC) $(CFLAGS) -c -o $@ $<

# --- 書き込み ---
flash: build
	@echo ""
	@echo "=== GR-SAKURA への書き込み開始 ==="
	@echo "注意: スライドスイッチ=WRITE, SW1(リセット)を押してから実行してください"
	$(RFP_CLI) -d RX63x -t usb -noverify-id -nocheck-range -e -p $(TARGET).mot -v
	@echo "=== 書き込み完了 ==="
	@echo "次: スライドスイッチを RUN に戻して SW1 を押してください"

# --- サイズ表示 ---
size: $(TARGET).elf
	$(SIZE) -A $<

# --- 逆アセンブル ---
disasm: $(TARGET).elf
	$(OBJDUMP) -d $< > $(TARGET).asm
	@echo ">>> 逆アセンブル出力: $(TARGET).asm"

# --- クリーン ---
clean:
	rm -f src/*.o generate/*.o freertos/*.o freertos/portable/GCC/RX600/*.o freertos/portable/MemMang/*.o
	rm -f $(TARGET).elf $(TARGET).mot $(TARGET).map $(TARGET).asm
	@echo "=== クリーン完了 ==="
