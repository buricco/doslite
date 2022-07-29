restore.exe:	restore.o
	wcl -fe=restore.exe restore.o

restore.o:	restore.c
	wcl -fo=restore.o -c restore.c

clean:
	rm -f restore.exe restore.o restore.err
