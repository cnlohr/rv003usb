all : testtop ledtest

testtop : testtop.c
	gcc -o $@ $^ -I../../ch32fun/minichlink -ludev

ledtest : ledtest.c
	gcc -o $@ $^ -I../../ch32fun/minichlink -ludev

clean :
	rm -rf *.o *~ testtop ledtest

