all : testtop ledtest

testtop : testtop.c
	gcc -o $@ $^ -I../../ch32v003fun/minichlink -ludev

ledtest : ledtest.c
	gcc -o $@ $^ -I../../ch32v003fun/minichlink -ludev

clean :
	rm -rf *.o *~ testtop ledtest

