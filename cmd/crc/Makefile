crc.exe:	crc.o
	wcl -k12288 -fe=crc.exe crc.o

crc.o:	crc.c
	wcl -k12288 -fo=crc.o -c crc.c

clean:
	rm -f crc.exe crc.o crc.err
