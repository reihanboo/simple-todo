CC = gcc
CFLAGS = -lpthread -Ilib -Isrc
SRC = src/main.c src/db.c
LIB_SRC = lib/sqlite3.c
OUT = build/td

TEST_SRC = tests/test_todo.c src/db.c
TEST_OUT = build/test_todo

all: $(OUT)

$(OUT): $(SRC) $(LIB_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(LIB_SRC) -o $(OUT)

test: $(TEST_OUT)
	./$(TEST_OUT)

$(TEST_OUT): $(TEST_SRC) $(LIB_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(TEST_SRC) $(LIB_SRC) -o $(TEST_OUT)

clean:
	rm -rf build/
