CC := arm-linux-gnueabihf-gcc
CFLAGS := -I../src -I../sdk -Icmocka/include/ -I../openssl/include -mthumb -W -Wall -DOS_LITTLE_ENDIAN -DNATIVE_64BITS -DST31
LDFLAGS := -static -L../src -L../openssl -lemu -lssl -lcrypto -ldl

all: hello test_aes test_crc16 test_ecdh test_hmac test_ripemd test_sha2

cmocka.o: cmocka/src/cmocka.c
	$(CC) $(CFLAGS) -DHAVE_MALLOC_H -DHAVE_INTTYPES_H -DHAVE_SIGNAL_H -DHAVE_STRINGS_H -o $@ -c $^

hello: hello.o
	$(CC) -o $@ $^ -static

test_aes: test_aes.o cmocka.o nist_cavp.o utils.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_crc16: test_crc16.o cmocka.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_ecdh: test_ecdh.o cmocka.o utils.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_hmac: test_hmac.o cmocka.o utils.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_ripemd: test_ripemd.o cmocka.o utils.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_sha2: test_sha2.o cmocka.o nist_cavp.o utils.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^
