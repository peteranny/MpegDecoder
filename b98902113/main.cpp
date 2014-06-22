#include "mpeg1.h"
MPEG1 mpeg;
int main(int argc, char *argv[]){
	mpeg.loadInfo("I_ONLY.m1v");
	mpeg.open("I_ONLY.m1v", 0);
	return 0;
}
