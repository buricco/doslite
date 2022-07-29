tree.exe:	tree.o
	wcl -fe=tree.exe tree.o

tree.o:	tree.c
	wcl -fo=tree.o -c tree.c

clean:
	rm -f tree.exe tree.o tree.err
