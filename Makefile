#IN=../MPEG/I_ONLY.M1V
#IN=../MPEG/IP_ONLY.M1V
#IN=../MPEG/IPB_ALL.M1V
#OUT=i_only
#OUT=ip_only
#OUT=ipb_all
OS=$(shell uname -s)
all:
	g++ --std=c++11 -o MpegDecoder src/main.cpp
run:
#	if[:q
#	OSiTYPiEcygwin)
#alias open="cmd /c start"
#linux)
#alias start="gnome-open"
#alias open="gnome-open"
#darwin*)
#alias start="open"

	./MpegDecoder $(IN)
clean:
	rm *~ .*
