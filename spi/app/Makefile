TARGET = source

ifeq (,$(TOOLCHAIN_PREFIX))
$(error TOOLCHAIN_PREFIX is not set)
endif

ifeq (,$(CFLAGS))
$(error CFLAGS is not set)
endif

ifeq (,$(LDFLAGS))
$(error LDFLAGS is not set)
endif

CC = $(TOOLCHAIN_PREFIX)gcc

CFLAGS += -I./app/inc

LDFLAGS += -L$(SYSROOT)/lib
LDFLAGS += -L$(SYSROOT)/usr/lib
LDFLAGS += -lwiringx

SOURCE = $(wildcard app/src/*.c)
OBJS = $(patsubst app/src/%.c,obj/%.o,$(SOURCE))

HEADERS = $(wildcard app/inc/*.h)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

obj/%.o: app/src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	@rm -rf obj/ $(TARGET)

# Create the obj directory if it doesn't exist
$(shell mkdir -p obj)
