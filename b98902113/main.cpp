#include <stdio.h>
#include "mpeg1.h"
int main(int argc, char *argv[]){

	fprintf(stderr, "%s\n", argv[1]);
	MPEG1 mpeg;
	mpeg.open(argv[1], 0);
}
