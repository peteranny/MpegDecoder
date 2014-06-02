IN=../MPEG/I_ONLY.M1V
all:
	g++ -g -o main main.cpp
	make run
run:
	./main $(IN)
clean:
	rm *~
ans:
	g++ MPEGDecoder.cpp
	./a.out $(IN)
