#include <stdio.h>
#include <string.h>
#include <stdint.h>

char cmd[1024];

int main(int argc, char ** argv) {
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			if (!strncmp(argv[i], "-m", 2)) {
				snprintf(cmd, sizeof(cmd), "%s -r + 0x1FFFF7E8 8 2>&1", argv[++i]);
			}
		}
	}

	FILE *fp;
	char buffer[1024];

	fp = popen(cmd, "r"); 
	if (fp == NULL) {
		printf("n/a\n");
		return -1;
	}
	int sn[8];
	uint8_t found = 0;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if(sscanf(buffer, "1ffff7e8: %x %x %x %x %x %x %x %x ", &sn[3], &sn[2], &sn[1], &sn[0], &sn[7], &sn[6], &sn[5], &sn[4]) > 4) {
			found++;
		}
	}
	if (found) printf("%02X%02X%02X%02X%02X%02X%02X%02X\n", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);
	else printf("n/a\n");

	int status = pclose(fp);

	return 0;
}