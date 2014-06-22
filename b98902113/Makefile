IN=I_ONLY.m1v
all:
	g++ -g -o main main.cpp mpeg1.cpp huffman.cpp
run:
	./main $(IN)

winall: winmain.cpp resource.o mpeg1.o huffman.o
	g++ winmain.cpp resource.o mpeg1.o huffman.o -o winmain.exe -lcomdlg32 -lgdi32 -O3 -static -lm -lwinmm

main.exe: main.cpp mpeg1.o huffman.o
	g++ main.cpp mpeg1.o huffman.o -o main.exe -O3

resource.o: resource.rc resource.h
	windres -i resource.rc -o resource.o

huffman.o: huffman.cpp huffman.h
	g++ huffman.cpp -o huffman.o -c -O3

mpeg1.o: mpeg1.cpp mpeg1.h
	g++ mpeg1.cpp -c -o mpeg1.o
