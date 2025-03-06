CC = gcc
CFLAGS = -lpthread
SRC = src/main.c
LIB_SRC = lib/sqlite3.c
OUT = build/td

# gcc main.c sqlite3.c -lpthread -o foo

all: $(OUT)

$(OUT): $(SRC) $(LIB_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(LIB_SRC) -o $(OUT)

clean:
	rm -rf build/
