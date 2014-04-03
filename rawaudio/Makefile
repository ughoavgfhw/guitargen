
CFLAGS += -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -O3

LDFLAGS += -O3

libaudio.so: $(patsubst %.c,%.o,$(wildcard *.c))
	$(LD) $(LDFLAGS) -L/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -shared -o $@ $^

audio_test: audio_test.c.txt libaudio.so
	$(CC) $(LDFLAGS) $(CFLAGS) ./libaudio.so -x c $< -o $@

clean:
	rm -f *.o libaudio.so libaudio.a audio_test *~
