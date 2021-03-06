#include <stdlib.h>
#include "MpegDecoder.h"
#ifndef EXIT
#define EXIT(msg) {fputs(msg, stderr);exit(0);}
#endif

void get_filename(char *inputName, char outputName[]){
	int head = 0, tail = strlen(inputName);
	for(int i = 0; i < strlen(inputName); i++){
		if(inputName[i] == '/' || inputName[i] == '\\'){ head = i + 1; tail = strlen(inputName); }
		else if(inputName[i] == '.'){ tail = i; }
	}
	for(int i = head, j = 0; i < tail; i++, j++){ outputName[j] = inputName[i]; }
	outputName[tail - head] = 0;
}
int main(int argc, char *argv[]){
	if(argc < 2) EXIT("Usage: main [input.m1v]");// [output_dir]");
	char *inputName = argv[1], outputName[1024];
	get_filename(inputName, outputName);//argv[2];
	MpegDecoder md(inputName, outputName);
	md.read();

	const char *html = 
	#include "player.txt"
	FILE *player = fopen("player.html", "w");
	int flag = 0;
	for(int i = 0; i < strlen(html); i++){
		if(html[i] == '?'){
			if(flag == 0) fprintf(player, "%s", outputName);
			if(flag == 1) fprintf(player, "%.2f", md.get_pps());
			flag++;
			continue;
		}
		fprintf(player, "%c", html[i]);
	}
	fclose(player);
	system("case $OSTYPE in\
			cygwin*) cmd /c start player.html;;\
			linux*) gnome-open player.html;;\
			darwin*) open player.html;;\
			esac");
//	system("case $OSTYPE in\
			cygwin*) alias open='cmd /c start';;\
			linux*) alias start='gnome-open';;\
			darwin*) alias start='open';;\
			esac;\
			start player.html");

	return 0;
}
