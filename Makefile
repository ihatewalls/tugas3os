CC     = gcc
CFLAGS = -Wall -Wextra -O2
SRC    = main.c banner.c
HDR    = banner.h

# menghasilkan kedua versi
all: dynamic static

# versi dinamis : libc dilink saat runtime
dynamic: mem_dynamic
mem_dynamic: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

# versi statis : libc disalin ke dalam biner (gcc -static)
static: mem_static
mem_static: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -static -o $@ $(SRC)

# hapus hasil build
clean:
	rm -f mem_static mem_dynamic

# menentukan bahwa nama di bawah adalah nama perintah bukan nama file
.PHONY: all static dynamic clean
