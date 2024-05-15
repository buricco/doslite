driver.sys:	driver.o
	wlink system dos com file driver.o name driver.sys

driver.o:	driver.asm msbds.inc
	wasm -fo=driver.o driver.asm

clean:
	rm -f driver.sys driver.o
