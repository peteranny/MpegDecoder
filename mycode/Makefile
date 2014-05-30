IN=../MPEG/I_ONLY.M1V
all:
	g++ -o main main.cpp
	make run
run:
	./main $(IN)
clean:
	rm *~
ans:
	g++ MPEGDecoder.cpp
	./a.out $(IN)
gitinit:
	git remote add origin http://github.com/peteranny/MpegDecoder.git
git:
	git add *
	git commit -m "$(MSG)"

	git push master

