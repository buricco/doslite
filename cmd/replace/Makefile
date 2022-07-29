replace.exe:	replace.o
	wcl -k12288 -fe=replace.exe replace.o

replace.o:	replace.c
	wcl -k12288 -fo=replace.o -c replace.c

clean:
	rm -f replace.exe replace.o replace.err
