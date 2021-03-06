const int seq2zz[] = {
	0, 1, 5, 6, 14, 15, 27, 28,
	2, 4, 7, 13, 16, 26, 29, 42,
	3, 8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
5, 36, 48, 49, 57, 58, 62, 63
};

class JpegDecoder{
	private:
		class Pixel{
			int r, g, b;
			int y, cb, cr;
		};
		Pixel *image;
		int frame_x;
		int frame_y;
	public:
		void set_size(int frame_x, int frame_y){
			this->frame_x = frame_x;
			this->frame_y = frame_y;
			image = new Pixel[frame_x*frame_y];
			return;
		}
	private:
		int dc_predictor;
	public:
		void reset_frame(){
			dc_predictor = 0;
		}
	private:
		int block[64];
		int block_i;
	public:
		void reset_block(){
			block_i = 0;
			for(int i = 0; i < 64; i++) block[i] = 0;
		}
		void set_dc(int dc){
			fprintf(stderr, "JpegDecoder.set_block_dc(): dc=%d\n", dc);
			block[block_i++] = dc_predictor += dc;
			return;
		}
		void set_ac(int run, int level){
			fprintf(stderr, "JpegDecoder.set_block_ac(): run=%d, level=%d.\n", run, level);
			for(int i = 0; i < run; i++){
				fprintf(stderr, "JpegDecoder.set_block_ac(): block_i=%d.\n", block_i);
				block[block_i++] = 0;
			}
			block[block_i++] = level;
			// TODO
			//
		}
		void dequant(int quant[]){
			for(int i = 0; i < 64; i++){
				block[i] *= quant[i];
			}
			return;
		}
		void print_block(bool isZz){
			for(int i = 0; i < 8; i++) for(int j = 0; j < 8; j++){
				fprintf(stderr, "%4d ",  block[isZz? seq2zz[8*i + j]: 8*i + j]);
			}
		}
};


