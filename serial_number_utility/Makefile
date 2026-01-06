all : serialnumberutility

# to make CI happy.
serialnumberutility.elf : serialnumberutility.c
	gcc -o $@ $^

serialnumberutility : serialnumberutility.c
	gcc -o $@ $^

clean :
	rm -rf *.o *~ serialnumberutility