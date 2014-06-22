#IN=../MPEG/I_ONLY.M1V
#IN=../MPEG/IP_ONLY.M1V
#IN=../MPEG/IPB_ALL.M1V
#OUT=i_only
all:
	g++ -o MpegDecoder src/main.cpp
	make run
run:
	./MpegDecoder $(IN) $(OUT)
clean:
	rm *~ .*
