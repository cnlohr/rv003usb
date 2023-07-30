all : testtop

testtop : testtop.c
	gcc -o $@ $^ -I../../ch32v003fun/minichlink -ludev

clean :
	rm -rf *.o *~ testtop

