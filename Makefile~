#IN=../MPEG/I_ONLY.M1V
#IN=../MPEG/IP_ONLY.M1V
IN=../MPEG/IPB_ALL.M1V
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
