#include <stdio.h>
#include <stdlib.h>

int main() {
	FILE *fp;
	fp = fopen("/dev/ArduinoSpidev", "w");
	if(fp == NULL){
		printf("error!");
		exit(1);
	}

	fprintf(fp, "a");

	fclose(fp);

	return 0;
}
