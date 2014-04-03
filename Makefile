CFLAGS += -O3 -g
LDFLAGS += -lm -lpthread -g -O3

guitargen: guitargen.o rawaudio/libaudio.so

guitargen.o: $(wildcard splits/*.c)

.PHONY: rawaudio/libaudio.so
rawaudio/libaudio.so:
	make -C rawaudio libaudio.so
