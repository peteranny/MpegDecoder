#include <cstdio>
#ifndef EXIT
#define EXIT(msg) {fputs(msg, stderr);exit(0);}
#endif

#ifndef FILEWRITER
#define FILEWRITER
class FileWriter{
	private:
		FILE *file;
	public:
		FileWriter(){}
		FileWriter(const char *filename){
			set_filename(filename);
		}
		void set_filename(const char *filename){
			if(file) fclose(file);
			file = fopen(filename, "wb");
			nBits = 0;
			if(!file) EXIT("FileWriter.set_filename(): error.");
		}
		int nBits;
		#define LITTLEENDIAN 0
		#define BIGENDIAN 1
		void write(int orderType, unsigned char *buf, int size){
			int i;
			switch(orderType){
				case LITTLEENDIAN:
					for(i = size - 1; i >= 0; i--){
						fputc(buf[i], file);
						nBits++;
						//fprintf(stderr, "%02X ", buf[i]);
					}
					break;
				case BIGENDIAN:
					for(i = 0; i < size; i++){
						fputc(buf[i], file);
						nBits++;
						//fprintf(stderr, "%02X ", buf[i]);
					}
					break;
				default:
					EXIT("FileWriter.write2file(): error.");
			}
			return;
		}
		void go2(int offset, int whereas){
			fseek(file, offset, whereas);
		}
		void close(){
			//fprintf(stderr, "FileWriter.write2file(): nBits=%d\n", nBits);
			fclose(file);
			file = NULL;
		}
};

#endif


