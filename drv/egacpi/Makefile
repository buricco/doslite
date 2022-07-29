all:	ega.cpi

ega.cpi:	egacpi.a86 cp437cga.i86 cp437ega.i86 cp437vga.i86 \
	cp850cga.i86 cp850ega.i86 cp850vga.i86 cp852cga.i86 cp852ega.i86 \
	cp852vga.i86 cp855cga.i86 cp855ega.i86 cp855vga.i86 cp860cga.i86 \
	cp860ega.i86 cp860vga.i86 cp863cga.i86 cp863ega.i86 cp863vga.i86
	nasm -o ega.cpi egacpi.a86

clean:
	rm -f ega.cpi
