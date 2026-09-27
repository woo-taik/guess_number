# 숫자 맞추기 게임 빌드 파일
#
#   make          : macOS/Linux 실행 파일 (guess)
#   make run      : 빌드 후 바로 실행
#   make windows  : Windows 실행 파일 (guess.exe) - mingw-w64 필요
#   make clean    : 생성물 삭제

CC       ?= cc
CFLAGS   ?= -Wall -Wextra -O2
TARGET    = guess
SRC       = guess.c

# Windows 크로스 컴파일 (Homebrew mingw-w64)
WINCC     = /usr/local/opt/mingw-w64/bin/x86_64-w64-mingw32-gcc
WINTARGET = guess.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: $(TARGET)
	./$(TARGET)

windows: $(WINTARGET)

$(WINTARGET): $(SRC)
	$(WINCC) $(CFLAGS) -static -o $@ $(SRC)

clean:
	rm -f $(TARGET) $(WINTARGET)

.PHONY: all run windows clean
