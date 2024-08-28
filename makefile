CC := gcc
CFLAGS := -std=c99 -Wall -Wextra

SRC_DIR := src
OBJ_DIR := obj
SOBJ_DIR := sobj
BIN_DIR := bin

# VERSION
MAJOR_VERSION := 0
MINOR_VERSION := 1
PATCH_VERSION := 0

LIB_NAME := libvvector-$(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION).so

.PHONY: build install demo uninstall clean

build: $(SRC_DIR)/vvector.c
	echo "Compiling $(LIB_NAME)"
	$(CC) $(CFLAGS) -c -shared -fPIC -o $(SOBJ_DIR)/$(LIB_NAME) $(SRC_DIR)/vvector.c
	echo "Done. Output is in $(SOBJ_DIR)/$(LIB_NAME)"

install: $(SOBJ_DIR)/$(LIB_NAME) $(SRC_DIR)/vvector.h
	sudo cp $(SOBJ_DIR)/$(LIB_NAME) /usr/local/lib/
	sudo ldconfig
	sudo cp $(SRC_DIR)/vvector.h /usr/local/include/
	sudo ldconfig
	echo "Done"

demo: $(SRC_DIR)/vvector.c $(SRC_DIR)/demo.c
	$(CC) $(CFLAGS) -g -c -o $(OBJ_DIR)/vvector.o $(SRC_DIR)/vvector.c
	$(CC) $(CFLAGS) -g -c -o $(OBJ_DIR)/demo.o $(SRC_DIR)/demo.c
	$(CC) $(CFLAGS) -g -o $(BIN_DIR)/demo $(OBJ_DIR)/demo.o $(OBJ_DIR)/vvector.o
	echo "Done. Run demo is at: ./$(BIN_DIR)/demo"

clean:
	rm -f $(SOBJ_DIR)/*
	rm -f $(BIN_DIR)/*
	rm -f $(OBJ_DIR)/*

uninstall:
	sudo rm -f /usr/local/lib/libvvector-*
	sudo ldconfig
	sudo rm -f /usr/local/include/vvector.h
	sudo ldconfig