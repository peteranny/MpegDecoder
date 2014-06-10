IN=../MPEG/I_ONLY.M1V
OUT=test
all:
	g++ -g -o main main.cpp
	make run
run:
	./main $(IN)
clean:
	rm *~ .*
ans: MPEGDecoder.cpp
	g++ -g MPEGDecoder.cpp
	./a.out $(IN) $(OUT)
