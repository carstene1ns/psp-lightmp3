PSPSDK=$(shell psp-config --pspsdk-path)
MPEG_OBJS = sceAsfParser.o sceAsfParser_0000.o sceAsfParser_0001.o sceAsfParser_0002.o sceAsfParser_0003.o sceAsfParser_0004.o \
	sceAsfParser_0005.o sceAsfParser_0006.o sceAsfParser_0007.o sceAsfParser_0008.o sceAsfParser_0009.o sceAsfParser_0010.o \
	sceAsfParser_0011.o sceAsfParser_0012.o sceAsfParser_0013.o

%.o: sceAsfParser.S
	psp-gcc -g -O2 -G0 -Wall -I${PSPSDK}/include -DF_$* $< -c -o $@

all: $(MPEG_OBJS)
	psp-ar cru ../../libpspasfparser.a $(MPEG_OBJS)
	psp-ranlib ../../libpspasfparser.a

clean:
	rm -f *.o
	rm -f *.bak
	rm -f ../../libpspasfparser.a
