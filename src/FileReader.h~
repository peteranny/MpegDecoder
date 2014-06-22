#include <cstdio>
#ifndef EXIT
#define EXIT(msg) {fputs(msg, stderr);exit(0);}
#endif

#ifndef FILEREADER
#define FILEREADER
class FileReader{
	private:
		FILE *file;
		unsigned char cur_byte;
		int cur_posi_in_byte;

	public:
		FileReader(){}
		FileReader(const char *filename){
			set_filename(filename);
		}
		void set_filename(const char *filename){
			file = fopen(filename, "rb");
			if(!file) EXIT("FileReader() error.");
			read(1, &cur_byte);
		}

	private:
		#define FILEBUFSIZE 1024
		unsigned char filebuf[FILEBUFSIZE];
		int filebuf_head;
		int filebuflen;
		
	public:
		void rtrn(int nBytes, unsigned char *buf){//unsigned char curByte){
			int i;
			for(i = nBytes - 1; i >= 0; i--){
				//fprintf(stderr, "FileReader.rtrn(): return %02X\n", buf[i]);
				if(filebuflen == FILEBUFSIZE - 1){
					EXIT("FileReader.rtrn() error.");
				}
				else{
					filebuf[filebuf_head] = cur_byte;
					filebuf_head = (filebuf_head + 1)%FILEBUFSIZE;
					filebuflen++;
		//fprintf(stderr, "retn2file : retnSize=%d, filebuf=", size);int j;for(j=filebuf_head-filebuflen;j<filebuf_head;j++)fprintf(stderr, "%02X",filebuf[j]);fprintf(stderr, ", i=%d, len=%d\n", filebuf_head, filebuflen);
				}
				cur_byte = buf[i];
			}
			//cur_byte = curByte;
			//fprintf(stderr, "FileReader.rtrn(): cur=%02X, posi=%d\n", cur_byte, cur_posi_in_byte);
			return;
		}

		bool read(int nBytes, unsigned char *buf){//, bool rmFF00 = false){
			//fprintf(stderr, "read(): nBytes=%d\n", nBytes);
			for(int i = 0; i < nBytes; i++){
				buf[i] = cur_byte;
				if(filebuflen == 0 && feof(file)){
					// meaning that cur_byte is EOF
					fprintf(stderr, "FileReader: read(): eof\n");
					return false;
				}

				// read from buffer
				if(filebuflen > 0){
					filebuf_head = (filebuf_head + FILEBUFSIZE - 1)%FILEBUFSIZE;
					cur_byte = filebuf[filebuf_head];
					filebuflen--;
					if(filebuflen < 0) EXIT("FileReader.read() error.");
				}
				// read from file
				else{
					cur_byte = fgetc(file);

					//fprintf(stderr, "read4mfile: fetch %02X\n", buf[i]);
					/*
					if(rmFF00){
						// look ahead
						if(buf[i] == 0xFF){
							//fprintf(stderr, "read4mfile: detect 0xFF\n");
							unsigned char next = fgetc(file);
							if(next == 0x00){ // discard //fprintf(stderr, "read4mfile: detect next byte=0x00\n");
							}
							else{
								rtrn(&next, 1);
							}
						}
					}
					*/
				}
				//fprintf(stderr, "FileReader.read(): buf[%d]=%02X, cur=%02X, posi=%d\n", i, buf[i], cur_byte, cur_posi_in_byte);
			}
			return true;
		}

		int read_bits(int nBits, unsigned char *buf){
			int nBytes = (nBits > 0)? (cur_posi_in_byte + nBits - 1)/8 + 1: 0;
			//fprintf(stderr, "FileReader.read_bits(): nBits=%d, nBytes=%d\n", nBits, nBytes);
			//if(size < nBytes_more) EXIT("FileReader.read_bits(): error.");
			if(!read(nBytes, buf)) return false;

			// clear bits before head
			//buf[0] &= ((unsigned char)0xFF) >> (cur_posi_in_byte);
			// clear bits after tail
			int ret = cur_posi_in_byte;
			cur_posi_in_byte = (cur_posi_in_byte + nBits)%8;
			if(cur_posi_in_byte > 0){
				rtrn(1, &buf[nBytes - 1]);//, buf[nBytes - 1]);
			}
			//buf[nBytes_more] &= ((unsigned char)0xFF) << (cur_posi_in_byte);

			//fprintf(stderr, "FileReader.read_bits(): cur=%02X, posi=%d\n", cur_byte, cur_posi_in_byte);
			return ret; // return starting position in buf[0]
		}

		int read_bits_as_num(int nBits){
			int nBytes = (nBits + 7)/8 + 1;
			unsigned char buf[nBytes];
			//fprintf(stderr, "FileReader.read_bits_as_num(): nBits=%d\n", nBits);
			int buf_head = read_bits(nBits, buf);
			int ret = bit2num(nBits, buf, buf_head);
			return ret;
		}

		int get_cur_posi_in_byte(){
			return cur_posi_in_byte;
		}
/*
void read4mfile_skip_ff00(FILE *file, unsigned char *byte, int dist, int rmFF00){
	if(dist <= filebuflen){
		filebuf_head = (filebuf_head + 1024 - dist)%FILEBUFSIZE;
		*byte = filebuf[filebuf_head];
		filebuflen -= dist;
		if(filebuflen < 0) EXIT("read4mfile_skip : error");
		//fprintf(stderr, "read4mfile_skip: fetch %02X\n", *byte);
	}
	else{
		int nByte2fetch = dist - filebuflen;
		filebuflen = 0;
		int i;
		for(i = 0; i < nByte2fetch; i++){
			read4mfile_ff00(file, byte, 1, rmFF00);
		}
	}
	return;
}
void read4mfile_skip(FILE *file, unsigned char *byte, int dist){
	read4mfile_skip_ff00(file, byte, dist, 0);
}
*/
};
#endif

