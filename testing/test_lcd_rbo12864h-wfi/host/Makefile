all : host

host : host.c
	gcc -g -o $@ $^ -I../../../ch32v003fun/minichlink -ludev -lusb-1.0 -lX11 -lXext -lGL -lm

install_rules :
	echo -ne "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", MODE=\"0664\", GROUP=\"plugdev\"\n" | sudo tee /etc/udev/rules.d/20-hidraw.rules
	sudo udevadm control --reload-rules
	sudo udevadm trigger

clean :
	rm -rf *.o *~ host

