CFLAGS += -O3
LDFLAGS += -lm -lpthread

guitargen: guitargen.o rawaudio/libaudio.so

.PHONY: rawaudio/libaudio.so
rawaudio/libaudio.so:
	make -C rawaudio libaudio.so
