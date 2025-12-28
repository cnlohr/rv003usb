all : testtop

testtop : testtop.c
	gcc -o $@ $^ -I../../ch32fun/minichlink -ludev

clean :
	rm -rf *.o *~ testtop

