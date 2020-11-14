#mingw32-make
#CXX = g++
#CXXFLAGS = -Wall -std=c++17 -O2
#CXXFLAGS += -fexec-charset=GBK -finput-charset=UTF-8
#CXXFLAGS += --target=i686-W64-windows-gnu
#LDFLAGS = -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread
#LDFLAGS = -static-libgcc -Wl,-Bstatic -lpthread -Wl,-Bdynamic -lws2_32 -s
#-Wl,-Bdynamic -lgcc_s

# 命令行范例 make CFLAG="-DNLOG -DNDEBUG" LDFLAG="-lpthread" CC=clang
CC = gcc
CFLAGS = $(CFLAG)
LDFLAGS = $(LDFLAG)
CFLAGS += -Wall -O2 -Ilibuv/include/
CFLAGS += -DHTTPCTX_PAGE_SIZE=1024 -DHS_CTX_POOL_SIZE=8 -DHS_HEAD_POOL_SIZE=64
#LDFLAGS += -s
#BITS 可选值 32/64, 指明是编译成32位程序还是64位程序

OSNAME = $(shell uname -s)
ifeq ($(OSNAME), Linux)
	# linux config, linux平台采用静态链接方式, 避免对libc的依赖
	LDFLAGS += -static -lpthread
	LIBUV = libuv/lib/libuv-linux-x64.a
else
	EXT = .exe
	LDFLAGS += -lws2_32 -lpsapi -liphlpapi -luserenv
	LIBUV = libuv/lib/libuv-win-x64.a
endif

#SOURCE = $(wildcard *.c)
#OBJS = $(patsubst %.c,%.o,$(SOURCE))
APP = accinfo

SOURCE = log.c memarray.c mempool.c \
	aes.c md5.c hex.c urlencode.c \
	http_parser.c httpctx.c httpserver.c \
	aidb.c main.c

# OBJS = $(patsubst %.c,$(OUTPUT)%.o,$(SOURCE))
OBJS = $(patsubst %.c,%.o,$(SOURCE))

all: $(APP)

$(APP): $(OBJS) $(LIBUV)
	$(CC) $(CFLAGS) -o $@$(EXT) $^ $(LDFLAGS)

dep:
	$(CC) -MM $(SOURCE)

# $(OBJS): %.o : %.c
# 	$(CC) $(CFLAGS) -c -o $@ $<

#gcc -MM *.c 自动生成依赖
log.o: log.c log.h
memarray.o: memarray.c memarray.h
mempool.o: mempool.c mempool.h

aes.o: aes.c aes.h
md5.o: md5.c md5.h
hex.o: hex.c hex.h
urlencode.o: urlencode.c urlencode.h

http_parser.o: http_parser.c http_parser.h
httpctx.o: httpctx.c httpctx.h memarray.h list.h mempool.h http_parser.h \
 log.h
httpserver.o: httpserver.c httpserver.h httpctx.h memarray.h list.h \
 mempool.h http_parser.h log.h

aidb.o: aidb.c sharedptr.h md5.h aes.h aidb.h
main.o: main.c platform.h sharedptr.h memarray.h str.h log.h aidb.h \
 httpserver.h httpctx.h list.h mempool.h http_parser.h

clean:
	rm -f $(OBJS) $(APP)$(EXT)
