MAKEFLAGS += -s -j4

DIST_DIR = ./dist
SRC_DIR = ./src

C_SRC = $(wildcard $(SRC_DIR)/*.c)
CPP_SRC = $(wildcard $(SRC_DIR)/*.cpp)
C_OBJ = $(patsubst %.c, $(DIST_DIR)/%.o, $(notdir $(C_SRC)))
CPP_OBJ = $(patsubst %.cpp, $(DIST_DIR)/%.o, $(notdir $(CPP_SRC)))

TARGET = skyre-hookcamera.dll
BIN_TARGET = $(DIST_DIR)/$(TARGET)

CC = gcc
CXX = g++

CFLAGS = -Wall -Werror -Wformat -g -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -static -flto -s -mavx
CFLAGS += -I./src/imgui-1.91.9b -I./src/imgui-1.91.9b/backends -I./src/MinHook

LFLAGS = -lgdi32 -ld3d9 -ldwmapi -ld3dcompiler -lstdc++
LFLAGS += -L./src/MinHook -lMinHook -L./src/imgui-1.91.9b -limgui -limgui_impl_win32 -limgui_impl_dx9

.PHONY: all clean libs clean_libs clean_all

all: libs
	@make $(BIN_TARGET)

clean_all: clean_libs clean

libs:
	@echo Compiling libraries ...
	@make -s -C ./src/imgui-1.91.9b all
	@make -s -C ./src/MinHook libMinHook.a

clean_libs:
	@make -s -C ./src/imgui-1.91.9b clean
	@make -s -C ./src/MinHook clean

$(BIN_TARGET): $(C_OBJ) $(CPP_OBJ)
	@echo Linking ...
	@$(CXX) $(CFLAGS) $^ -shared -o $@ $(LFLAGS)

$(DIST_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $< ...
	@$(CC) $(CFLAGS) -c $< -o $@

$(DIST_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo Compiling $< ...
	@$(CXX) $(CFLAGS) -c $< -o $@

clean:
	@del .\dist\*.o
	@del .\dist\*.dll
