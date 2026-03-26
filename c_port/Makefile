CC := gcc
CFLAGS := -O3 -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS := -lm
CPPFLAGS := -Iinclude
SEED ?=

SRC_DIR := src
BIN_DIR := bin
OUT_DIR := out
OUT_CSV_DIR := $(OUT_DIR)/csv
OUT_SVG_DIR := $(OUT_DIR)/svg
CSV_PROP := $(OUT_CSV_DIR)/propagation
CSV_8 := $(OUT_CSV_DIR)/8psk
CSV_64 := $(OUT_CSV_DIR)/64apsk
CSV_256 := $(OUT_CSV_DIR)/256apsk
SVG_PROP := $(OUT_SVG_DIR)/propagation
SVG_8 := $(OUT_SVG_DIR)/8psk
SVG_64 := $(OUT_SVG_DIR)/64apsk
SVG_256 := $(OUT_SVG_DIR)/256apsk

TARGET := $(BIN_DIR)/propagation_losses
SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/propagation_losses.c
PLOT_TARGET := $(BIN_DIR)/plot_propagation_svg
PLOT_SRCS := $(SRC_DIR)/plot_propagation_svg.c
SIM8_TARGET := $(BIN_DIR)/simulate_8psk
SIM8_SRCS := $(SRC_DIR)/simulate_8psk.c $(SRC_DIR)/rrc_link.c
PLOT8_TARGET := $(BIN_DIR)/plot_8psk_svg
PLOT8_SRCS := $(SRC_DIR)/plot_8psk_svg.c
SIMAPSK_TARGET := $(BIN_DIR)/simulate_apsk
SIMAPSK_SRCS := $(SRC_DIR)/simulate_apsk.c $(SRC_DIR)/rrc_link.c
PLOTAPSK_TARGET := $(BIN_DIR)/plot_apsk_svg
PLOTAPSK_SRCS := $(SRC_DIR)/plot_apsk_svg.c
PLOTSPEC_TARGET := $(BIN_DIR)/plot_spectrum_svg
PLOTSPEC_SRCS := $(SRC_DIR)/plot_spectrum_svg.c

.PHONY: all clean run plot sim8 plot8 sim64 sim256 plot64 plot256 plotapsk plotall dirs tidy

dirs:
	mkdir -p $(BIN_DIR) $(CSV_PROP) $(CSV_8) $(CSV_64) $(CSV_256) $(SVG_PROP) $(SVG_8) $(SVG_64) $(SVG_256)

all: dirs $(TARGET) $(PLOT_TARGET) $(SIM8_TARGET) $(PLOT8_TARGET) $(SIMAPSK_TARGET) $(PLOTAPSK_TARGET) $(PLOTSPEC_TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

$(PLOT_TARGET): $(PLOT_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(PLOT_SRCS) $(LDFLAGS)

$(SIM8_TARGET): $(SIM8_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SIM8_SRCS) $(LDFLAGS)

$(PLOT8_TARGET): $(PLOT8_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(PLOT8_SRCS) $(LDFLAGS)

$(SIMAPSK_TARGET): $(SIMAPSK_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SIMAPSK_SRCS) $(LDFLAGS)

$(PLOTAPSK_TARGET): $(PLOTAPSK_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(PLOTAPSK_SRCS) $(LDFLAGS)

$(PLOTSPEC_TARGET): $(PLOTSPEC_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(PLOTSPEC_SRCS) $(LDFLAGS)

run: all
	cd $(CSV_PROP) && ../../..//$(TARGET)

plot: all
	cd $(CSV_PROP) && ../../..//$(TARGET) && ../../..//$(PLOT_TARGET)
	mv -f $(CSV_PROP)/propagation_losses.svg $(SVG_PROP)/

sim8: all
	cd $(CSV_8) && ../../..//$(SIM8_TARGET) $(SEED)

plot8: all
	cd $(CSV_8) && ../../..//$(SIM8_TARGET) $(SEED) && ../../..//$(PLOT8_TARGET) && ../../..//$(PLOTSPEC_TARGET) 8
	mv -f $(CSV_8)/*.svg $(SVG_8)/

sim64: all
	cd $(CSV_64) && ../../..//$(SIMAPSK_TARGET) 64 $(SEED)

sim256: all
	cd $(CSV_256) && ../../..//$(SIMAPSK_TARGET) 256 $(SEED)

plot64: all
	cd $(CSV_64) && ../../..//$(SIMAPSK_TARGET) 64 $(SEED) && ../../..//$(PLOTAPSK_TARGET) 64 && ../../..//$(PLOTSPEC_TARGET) 64
	mv -f $(CSV_64)/*.svg $(SVG_64)/

plot256: all
	cd $(CSV_256) && ../../..//$(SIMAPSK_TARGET) 256 $(SEED) && ../../..//$(PLOTAPSK_TARGET) 256 && ../../..//$(PLOTSPEC_TARGET) 256
	mv -f $(CSV_256)/*.svg $(SVG_256)/

plotapsk: all
	$(MAKE) plot64
	$(MAKE) plot256

plotall: all
	$(MAKE) plot
	$(MAKE) plot8
	$(MAKE) plot64
	$(MAKE) plot256

tidy:
	@echo "Binaries: $(BIN_DIR)/"
	@echo "CSV outputs: $(OUT_CSV_DIR)/{propagation,8psk,64apsk,256apsk}"
	@echo "SVG outputs: $(OUT_SVG_DIR)/{propagation,8psk,64apsk,256apsk}"

clean:
	rm -rf $(BIN_DIR) $(OUT_DIR)
