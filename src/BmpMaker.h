#include <stdio.h>
#include <string.h>
#include "FileWriter.h"
//#include "libbmpFile.h"

#ifndef BMPMAKER
#define BMPMAKER

#ifndef EXIT
#define EXIT(msg) {fputs(msg, stderr);exit(0);}
#endif

class BmpMaker{
	private:
		FileWriter bmpFile;

		int file_header_size;
		int file_size;

		int dib_header_size;
		int map_width;
		int map_height;
		int px_start;
		int map_size;
		int num_bits_per_px;
		int num_bytes_per_row;

	private:
		void go2posi(int row, int col){
			if(row >= map_height || col >= map_width) EXIT("BmpMaker.go2posi(): error.");
			bmpFile.go2(px_start, SEEK_SET);
			int offset = (map_height - row - 1)*num_bytes_per_row + col*num_bits_per_px/8;
			bmpFile.go2(offset, SEEK_CUR);
			return;
		}
		void put_px(int r[], int g[], int b[], int num){
			unsigned char px[4];
			int i;
			switch(num_bits_per_px){
				case 24:
				{
					for(i = 0; i < num; i++){
						px[0] = b[i];
						px[1] = g[i];
						px[2] = r[i];
						bmpFile.write(BIGENDIAN, px, 3);
					}
					int nPadding = num_bytes_per_row - 3*num;
					for(i = 0; i < nPadding; i++) px[i] = 0;
					bmpFile.write(BIGENDIAN, px, nPadding);
					return;
				}
				default:
					EXIT("BmpMaker.put_px(): error.");
			}
			return;
		}
	public:
		void make(const char *filepath, int *r, int *g, int *b, int width, int height){
			//fprintf(stderr, "Making bmp bmpFile...\n");
			bmpFile.set_filepath(filepath);

			/*
			int test_r[] = {0, 0, 0, 0};
			int test_g[] = {0, 0, 0, 0};
			int test_b[] = {0, 0, 0, 0};
			r = test_r;
			g = test_g;
			b = test_b;
			width = 2;
			height = 2;
			*/

			unsigned char buf[1024];

			// Bitmap file header starts
			file_header_size = 14;
			bmpFile.go2(0, SEEK_SET);

			// [0, 2]: header
			memcpy(buf, "BM", 2);
			bmpFile.write(BIGENDIAN, buf, 2);

			// [2, 6]: overall bmp=file size
			bmpFile.go2(4, SEEK_CUR); // come back later...

			// [6, 8]: reserved
			bmpFile.go2(2, SEEK_CUR);

			// [8, 10]: reserved
			bmpFile.go2(2, SEEK_CUR);

			// [10, 14]: starting address of the pixel array
			bmpFile.go2(4, SEEK_CUR); // come back later...

			// DIB header starts
			// [14, 18]: size of DIB header
			dib_header_size = 40;
			num2byte(dib_header_size, buf, 4);
			bmpFile.write(LITTLEENDIAN, buf, 4);

			// [18, 22]: width (signed int)
			map_width = width;
			num2byte(map_width, buf, 4);
			bmpFile.write(LITTLEENDIAN, buf, 4);

			// [22, 26]: height (signed int)
			map_height = height;
			num2byte(map_height, buf, 4);
			bmpFile.write(LITTLEENDIAN, buf, 4);

			// [26, 28]: the number of color planes must be 1
			num2byte(1, buf, 2);
			bmpFile.write(LITTLEENDIAN, buf, 2);

			// [28, 30]: the number of bits per pixel
			num_bits_per_px = 8*3; // RGB
			num2byte(num_bits_per_px, buf, 2);
			bmpFile.write(LITTLEENDIAN, buf, 2);

			// [30, 34]: the compression method being used
			bmpFile.go2(4, SEEK_CUR);

			// [34, 38]: the size of the raw bitmap data
			bmpFile.go2(4, SEEK_CUR); // come back later...

			// [38, 42]: the horizontal resolution of the image (pixel per meter, signed integer)
			bmpFile.go2(4, SEEK_CUR);

			// [42, 46]: the vertical resolution of the image. (pixel per meter, signed integer)
			bmpFile.go2(4, SEEK_CUR);

			// [46, 50]: the number of colors in the color palette
			bmpFile.go2(4, SEEK_CUR);

			// [50, 54]: the number of important colors used
			num2byte(0, buf, 4);
			bmpFile.write(LITTLEENDIAN, buf, 4);

			// rewind to fill in [10, 14]: starting address of the pixel array
			bmpFile.go2(10, SEEK_SET);
			px_start = file_header_size + dib_header_size;
			num2byte(px_start, buf, 4);
			bmpFile.write(LITTLEENDIAN, buf, 4);

			// [54, end]: pixel array
			bmpFile.go2(px_start, SEEK_SET);
			map_size = 0;

			int num_4Bytes_per_row = (num_bits_per_px*map_width + 31)/32;
			num_bytes_per_row = num_4Bytes_per_row*4;

			int i;
			for(i = 0; i < map_height; i++){
				go2posi(i, 0);
				put_px(&r[i*map_width], &g[i*map_width], &b[i*map_width], map_width);
				map_size += num_bytes_per_row;
			}

			// rewind to fill in [2, 6]: overall bmp=file size
			bmpFile.go2(2, SEEK_SET);
			file_size = file_header_size + dib_header_size + map_size;
			num2byte(file_size, buf, 4);
			bmpFile.write(LITTLEENDIAN, buf, 4);

			// rewind to fill in [34, 38]: the size of the raw bitmap data
			bmpFile.go2(34, SEEK_SET);
			num2byte(map_size, buf, 4);
			bmpFile.write(LITTLEENDIAN, buf, 4);

			//fprintf(stderr, "BmpMaker.make(): Done.\n");
			bmpFile.close();

			//TODO
			return;
		}
};

#endif
