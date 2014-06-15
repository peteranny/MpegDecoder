int frame_i;
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include "libbit.h"
#include "FileReader.h"
#include "BmpMaker.h"
#include "Huffman.h"
#ifndef EXIT
#define EXIT(msg) {fputs(msg, stderr);exit(0);}
#endif

#define PICTURE_START_CODE   0x00000100
#define SLICE_START_CODE(x)  ((0x01 <= ((x)&0x000000FF)) && (((x)&0x000000FF) <= 0xAF))
#define USER_DATA_START_CODE 0x000001B2
#define SEQUENCE_HEADER_CODE 0x000001B3
#define EXTENSION_START_CODE 0x000001B5
#define SEQUENCE_END_CODE    0x000001B7
#define GROUP_START_CODE     0x000001B8
class MpegDecoder{
	private:
		FileReader mpegFile;
		BmpMaker bmpMaker;
		const char *dir_name;
	public:
		MpegDecoder(const char *mpeg_name, const char *dir_name = "test"){
			mpegFile.set_filename(mpeg_name);
			this->dir_name = dir_name;
			mkdir(dir_name, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
		}
	private:
		class Pixel{
			public:
				int r, g, b;
				int y, cb, cr;
				void make_rgb(){
					r = (int)round(1.16438356164*(y - 16.0) + 1.59602678571*(cr - 128.0));
					g = (int)round(1.16438356164*(y - 16.0) - 0.39176229009*(cb - 128.0) - 0.81296764723*(cr - 128.0));
					b = (int)round(1.16438356164*(y - 16.0) + 2.01723214286*(cb - 128.0));
					if(r > 255) r = 255;
					if(g > 255) g = 255;
					if(b > 255) b = 255;
					if(r < 0) r = 0;
					if(g < 0) g = 0;
					if(b < 0) b = 0;
				}
				void reset(){
					r = g = b = y = cb = cr = 0;
				}
				friend Pixel operator+(const Pixel &p1, const Pixel &p2){
					Pixel p;
					p.y = p1.y + p2.y;
					p.cb = p1.cb + p2.cb;
					p.cr = p1.cr + p2.cr;
					p.r = p1.r + p2.r;
					p.g = p1.g + p2.g;
					p.b = p1.b + p2.b;
					return p;
				}
				friend Pixel &operator+=(Pixel &p1, const Pixel &p2){
					p1 = p1 + p2;
					return p1;
				}
				friend Pixel operator/(const Pixel &p1, const int i2){
					Pixel p;
					p.y = p1.y/i2;
					p.cb = p1.cb/i2;
					p.cr = p1.cr/i2;
					p.r = p1.r/i2;
					p.g = p1.g/i2;
					p.b = p1.b/i2;
					return p;
				}
				friend Pixel &operator/=(Pixel &p1, const int i2){
					p1 = p1/i2;
					return p1;
				}
		};
		int sequence_header_code;
		int horizontal_size;
		int vertical_size;
		int mb_width;
		int mb_height;
		Pixel **frame;
		Pixel **frame_past;
		Pixel **frame_future;
		int pel_aspect_ratio;
		int picture_rate;
		int bit_rate;
		int marker_bit;
		int vbv_buffer_size;
		int constrained_parameter_flag;
		int load_intra_quantizer_matrix;
		int intra_quantizer_matrix[8][8];
		int load_non_intra_quantizer_matrix;
		int non_intra_quantizer_matrix[8][8];
	public:
		void read_sequence_header(){
			sequence_header_code = mpegFile.read_bits_as_num(4*8);
			if(sequence_header_code != SEQUENCE_HEADER_CODE){
				EXIT("MpegDecoder.read_sequence_header(): error");
			}

			horizontal_size = mpegFile.read_bits_as_num(12);
			vertical_size = mpegFile.read_bits_as_num(12);
			
			// initialize an image frame
			mb_width = (horizontal_size + 15)/16;
			mb_height = (vertical_size + 15)/16;
			frame = new Pixel*[vertical_size];
			for(int i = 0; i < vertical_size; i++) frame[i] = new Pixel[horizontal_size];
			frame_past = new Pixel*[vertical_size];
			for(int i = 0; i < vertical_size; i++) frame_past[i] = new Pixel[horizontal_size];
			frame_future = new Pixel*[vertical_size];
			for(int i = 0; i < vertical_size; i++) frame_future[i] = new Pixel[horizontal_size];
			dct_zz = new int[64];
			dct_recon = new int*[8];
			for(int j = 0; j < 8; j++) dct_recon[j] = new int[8];

			pel_aspect_ratio = mpegFile.read_bits_as_num(4);
			// UNDONE

			picture_rate = mpegFile.read_bits_as_num(4);
			// UNDONE

			bit_rate = mpegFile.read_bits_as_num(18);
			// UNKNOWN

			marker_bit = mpegFile.read_bits_as_num(1);
			// UNDONE

			vbv_buffer_size = mpegFile.read_bits_as_num(10);
			// UNDONE

			constrained_parameter_flag = mpegFile.read_bits_as_num(1);
			// UNDONE

			load_intra_quantizer_matrix = mpegFile.read_bits_as_num(1);
			if(load_intra_quantizer_matrix){
				for(int j = 0; j < 64; j++){
					int k = scan_reverse[j];
					intra_quantizer_matrix[k/8][k%8] = mpegFile.read_bits_as_num(8);
				}
			}
			else{
				static int default_intra_quantizer_matrix[8][8] = {
					{8, 16, 19, 22, 26, 27, 29, 34},
					{16, 16, 22, 24, 27, 29, 34, 37},
					{19, 22, 26, 27, 29, 34, 34, 38},
					{22, 22, 26, 27, 29, 34, 37, 40},
					{22, 26, 27, 29, 32, 35, 40, 48},
					{26, 27, 29, 32, 35, 40, 48, 58},
					{26, 27, 29, 34, 38, 46, 56, 69},
					{27, 29, 35, 38, 46, 56, 69, 83}
				};
				for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
					intra_quantizer_matrix[m][n] = default_intra_quantizer_matrix[m][n];
				}
			}

			load_non_intra_quantizer_matrix = mpegFile.read_bits_as_num(1);
			if(load_non_intra_quantizer_matrix){
				for(int j = 0; j < 64; j++){
					int k = scan_reverse[j];
					non_intra_quantizer_matrix[k/8][k%8] = mpegFile.read_bits_as_num(8);
				}
			}
			else{
				static int default_non_intra_quantizer_matrix[8][8] = {
					{16, 16, 16, 16, 16, 16, 16, 16},
					{16, 16, 16, 16, 16, 16, 16, 16},
					{16, 16, 16, 16, 16, 16, 16, 16},
					{16, 16, 16, 16, 16, 16, 16, 16},
					{16, 16, 16, 16, 16, 16, 16, 16},
					{16, 16, 16, 16, 16, 16, 16, 16},
					{16, 16, 16, 16, 16, 16, 16, 16},
					{16, 16, 16, 16, 16, 16, 16, 16}
				};
				for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
					non_intra_quantizer_matrix[m][n] = default_non_intra_quantizer_matrix[m][n];
				}
			}

			next_start_code();
			return;
		}
	private:
		int group_start_code;
		int drop_frame_flag;
		int time_code_hours;
		int time_code_minutes;
		int time_code_seconds;
		int time_code_pictures;
		int closed_gop;
		int broken_link;
	public:
		void read_group_start(){
			group_start_code = mpegFile.read_bits_as_num(4*8);
			if(group_start_code != GROUP_START_CODE){
				EXIT("MpegDecoder.read_group_start(): error");
			}

			// time_code
			drop_frame_flag = mpegFile.read_bits_as_num(1);
			time_code_hours = mpegFile.read_bits_as_num(5);
			time_code_minutes = mpegFile.read_bits_as_num(6);
			marker_bit = mpegFile.read_bits_as_num(1); // should be 1
			time_code_seconds = mpegFile.read_bits_as_num(6);
			time_code_pictures = mpegFile.read_bits_as_num(6);
			// UNKNOWN
		
			closed_gop = mpegFile.read_bits_as_num(1);
			// UNDONE: no reference to previous groups, for editting
			
			broken_link = mpegFile.read_bits_as_num(1);
			// UNDONE: reference group not available, for editting
			
			next_start_code();
			return;
		}
	private:
		int picture_start_code;
		int temporal_reference;
		int picture_coding_type;
		int vbv_delay;
		int full_pel_forward_vector;
		int forward_f_code;
		int forward_r_size;
		int forward_f;
		int full_pel_backward_vector;
		int backward_f_code;
		int backward_r_size;
		int backward_f;
		int extra_bit_picture;
		int extra_information_picture;
	public:
		void read_picture_start(){
			picture_start_code = mpegFile.read_bits_as_num(4*8);
			if(picture_start_code != PICTURE_START_CODE){
				EXIT("MpegDecoder.read_picture_start(): error");
			}

			for(int i = 0; i < vertical_size; i++) for(int j = 0; j < horizontal_size; j++){
				frame[i][j].y = 0;
				frame[i][j].cb = 0;
				frame[i][j].cr = 0;
				frame[i][j].r = 0;
				frame[i][j].g = 0;
				frame[i][j].b = 0;
			}

			temporal_reference = mpegFile.read_bits_as_num(10);
			fprintf(stderr, "read_picture_start(): temporal_reference=%d\n", temporal_reference);

			picture_coding_type = mpegFile.read_bits_as_num(3);
			fprintf(stderr, "read_picture_start(): picture_coding_type=%d\n", picture_coding_type);

			vbv_delay = mpegFile.read_bits_as_num(16);
			fprintf(stderr, "read_picture_start(): vbv_delay=%d\n", vbv_delay);
			// UNDONE
			
			// I-frame or B-frame
			if(picture_coding_type == 2 || picture_coding_type == 3){
				full_pel_forward_vector = mpegFile.read_bits_as_num(1);
				fprintf(stderr, "read_picture_start(): full_pel_forward_vector=%d\n", full_pel_forward_vector);
				forward_f_code = mpegFile.read_bits_as_num(3);
				fprintf(stderr, "read_picture_start(): forward_f_code=%d\n", forward_f_code);
				forward_r_size = forward_f_code - 1;
				forward_f = 1 << forward_r_size;
			}

			// B-frame
			if(picture_coding_type == 3){
				full_pel_backward_vector = mpegFile.read_bits_as_num(1);
				backward_f_code = mpegFile.read_bits_as_num(3);
				fprintf(stderr, "read_picture_start(): backward_f_code=%d\n", backward_f_code);
				backward_r_size = backward_f_code - 1;
				backward_f = 1 << backward_r_size;
			}

			while(nextbits(1) == 1){ // b1
				extra_bit_picture = mpegFile.read_bits_as_num(1);
				fprintf(stderr, "read_picture_start(): extra_bit_picture=%d\n", extra_bit_picture);
				// UNDONE
				
				extra_information_picture = mpegFile.read_bits_as_num(8);
				// UNDONE
			}
			extra_bit_picture = mpegFile.read_bits_as_num(1);
			fprintf(stderr, "read_picture_start(): extra_bit_picture=%d\n", extra_bit_picture);
	
			next_start_code();
			return;
		}
	private:
		int slice_start_code;
		int slice_vertical_position;
		int quantizer_scale;
		int extra_bit_slice;
		int extra_information_slice;
	public:
		void read_slice_start(){
			slice_start_code = mpegFile.read_bits_as_num(4*8);
			if(!SLICE_START_CODE(slice_start_code)){
				EXIT("MpegDecoder.read_slice_start(): error");
			}
			slice_vertical_position = slice_start_code&0x000000FF;
			fprintf(stderr, "read_slice_start(): slice_start_code=%d (slice_vertical_position=%d)\n", slice_start_code, slice_vertical_position);

			quantizer_scale = mpegFile.read_bits_as_num(5);
			fprintf(stderr, "read_slice_start(): quantizer_scale=%d\n", quantizer_scale);
			// initailize
			previous_macroblock_address = (slice_vertical_position - 1)*mb_width - 1;
			past_intra_address = -2;
			dct_dc_y_past = 128*8;
			dct_dc_cb_past = 128*8;
			dct_dc_cr_past = 128*8;
			recon_right_for_prev = 0;
			recon_down_for_prev = 0;
			recon_right_back_prev = 0;
			recon_down_back_prev = 0;

			while(nextbits(1) == 1){ // b1
				extra_bit_slice = mpegFile.read_bits_as_num(1);
				fprintf(stderr, "read_slice_start(): extra_bit_slice=%d\n", extra_bit_slice);
				extra_information_slice = mpegFile.read_bits_as_num(8);
				// UNDONE
			}
			extra_bit_slice = mpegFile.read_bits_as_num(1);
			fprintf(stderr, "read_slice_start(): extra_bit_slice=%d\n", extra_bit_slice);

			if(picture_coding_type != 3){
				for(int i = 0; i < vertical_size; i++){
					memcpy(frame_past[i], frame_future[i], sizeof(Pixel)*horizontal_size);
				}
			}

			while(nextbits(23) != 0){ // b00000000000000000000000
				fprintf(stderr, "read_slice_start(): new macroblock...\n");
				read_macroblock();
			}

			// make R, G, B
			int r[vertical_size][horizontal_size];
			int g[vertical_size][horizontal_size];
			int b[vertical_size][horizontal_size];
			for(int i = 0; i < vertical_size; i++) for(int j = 0; j < horizontal_size; j++){
				frame[i][j].make_rgb();
				r[i][j] = frame[i][j].r;
				g[i][j] = frame[i][j].g;
				b[i][j] = frame[i][j].b;
			}

			// generate bmp file.
			char bmppath[1024];
			sprintf(bmppath, "%s/%d.bmp", dir_name, frame_i);
			bmpMaker.make(bmppath, &r[0][0], &g[0][0], &b[0][0], horizontal_size, vertical_size);

			// buffer for non intra frame
			if(picture_coding_type == 2){
				for(int i = 0; i < vertical_size; i++){
					memcpy(frame_future[i], frame[i], sizeof(Pixel)*horizontal_size);
				}
			}

			next_start_code();
			return;
		}
	private:
		int huffman_decode(Huffman *h){
			int ret = h->decode(nextbits(h->get_maxLen()));
			mpegFile.read_bits_as_num(h->get_codelen(ret));
			return ret;
		}
		int macroblock_address_increment;
		int macroblock_address;
		int previous_macroblock_address;
		int past_intra_address;
		int mb_row;
		int mb_column;
		int macroblock_type;
		int macroblock_quant;
		int macroblock_motion_forward;
		int macroblock_motion_backward;
		int macroblock_pattern;
		int macroblock_intra;

		int pattern_code[6];
		int coded_block_pattern;
		int recon_right_for;
		int recon_down_for;
		int recon_right_for_prev;
		int recon_down_for_prev;
		int recon_right_back;
		int recon_down_back;
		int recon_right_back_prev;
		int recon_down_back_prev;
	public:
		void read_macroblock(){
			while(true){
				static Huffman *macroblock_address_increment_huff = NULL;
				if(!macroblock_address_increment_huff){
					#include "macroblock_address_increment_huff.h"
				}
				macroblock_address_increment = huffman_decode(macroblock_address_increment_huff);
				fprintf(stderr, "read_macroblock(): macroblock_address_increment=%d\n", macroblock_address_increment);
				if(macroblock_address_increment == 0x00){
					continue;
				}
				if(macroblock_address_increment == 0xFF){
					macroblock_address += 33;
					continue;
				}
				break;
			}

			for(int j = 0; j < macroblock_address_increment; j++){
				macroblock_address = previous_macroblock_address + 1;
				fprintf(stderr, "read_macroblock(): macroblock_address=%d\n", macroblock_address);
				mb_row = macroblock_address/mb_width;
				mb_column = macroblock_address%mb_width;
				bool skipped_macroblock = j < macroblock_address_increment - 1;
				// process macroblock
				if(!skipped_macroblock){
					switch(picture_coding_type){
						case 1: // intra-coded (I)
							static Huffman *macroblock_type_I_huff = NULL;
							if(!macroblock_type_I_huff){
								#include "macroblock_type_I_huff.h"
							}
							macroblock_type = huffman_decode(macroblock_type_I_huff);
							switch(macroblock_type){
								case 1: // 1
									macroblock_quant = 0;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 1;
									break;
								case 2: // 01
									macroblock_quant = 1;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 1;
									break;
								default:
									EXIT("MpegDecoder.read_macroblock(): error. // macroblock_type_I");
							}
							break;
						case 2: // predictive-coded (P)
							static Huffman *macroblock_type_P_huff = NULL;
							if(!macroblock_type_P_huff){
								#include "macroblock_type_P_huff.h"
							}
							macroblock_type = huffman_decode(macroblock_type_P_huff);
							fprintf(stderr, "read_macroblock(): macroblock_type=");macroblock_type_P_huff->print_codeword(macroblock_type);fprintf(stderr, "\n");
							switch(macroblock_type){
								case 1: // 1
									macroblock_quant = 0;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 0;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 2: // 01
									macroblock_quant = 0;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 3: // 001
									macroblock_quant = 0;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 0;
									break;
								case 4: // 00011
									macroblock_quant = 0;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 1;
									break;
								case 5: // 00010
									macroblock_quant = 1;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 0;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 6: // 00001
									macroblock_quant = 1;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 7: // 000001
									macroblock_quant = 1;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 1;
									break;
							}
							fprintf(stderr, "read_macroblock(); macroblock_quant=%d\n", macroblock_quant);
							fprintf(stderr, "read_macroblock(); macroblock_motion_forward=%d\n", macroblock_motion_forward);
							fprintf(stderr, "read_macroblock(); macroblock_motion_backward=%d\n", macroblock_motion_backward);
							fprintf(stderr, "read_macroblock(); macroblock_pattern=%d\n", macroblock_pattern);
							fprintf(stderr, "read_macroblock(); macroblock_intra=%d\n", macroblock_intra);
							break;
						case 3: // bidirectionally predictive-coded (B)
							static Huffman *macroblock_type_B_huff = NULL;
							if(!macroblock_type_B_huff){
								#include "macroblock_type_B_huff.h"
							}
							macroblock_type = huffman_decode(macroblock_type_B_huff);
							fprintf(stderr, "read_macroblock(): macroblock_type=");macroblock_type_B_huff->print_codeword(macroblock_type);fprintf(stderr, "\n");
							switch(macroblock_type){
								case 1: // 10
									macroblock_quant = 0;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 1;
									macroblock_pattern = 0;
									macroblock_intra = 0;
									break;
								case 2: // 11
									macroblock_quant = 0;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 1;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 3: // 010
									macroblock_quant = 0;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 1;
									macroblock_pattern = 0;
									macroblock_intra = 0;
									break;
								case 4: // 011
									macroblock_quant = 0;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 1;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 5: // 0010
									macroblock_quant = 0;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 0;
									break;
								case 6: // 0011
									macroblock_quant = 0;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 0;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 7: // 00011
									macroblock_quant = 0;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 1;
									break;
								case 8: // 00010
									macroblock_quant = 1;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 1;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 9: // 000011
									macroblock_quant = 1;
									macroblock_motion_forward = 1;
									macroblock_motion_backward = 0;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 10: // 000010
									macroblock_quant = 1;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 1;
									macroblock_pattern = 1;
									macroblock_intra = 0;
									break;
								case 11: // 000001
									macroblock_quant = 1;
									macroblock_motion_forward = 0;
									macroblock_motion_backward = 0;
									macroblock_pattern = 0;
									macroblock_intra = 1;
									break;
							}
							fprintf(stderr, "read_macroblock(); macroblock_quant=%d\n", macroblock_quant);
							fprintf(stderr, "read_macroblock(); macroblock_motion_forward=%d\n", macroblock_motion_forward);
							fprintf(stderr, "read_macroblock(); macroblock_motion_backward=%d\n", macroblock_motion_backward);
							fprintf(stderr, "read_macroblock(); macroblock_pattern=%d\n", macroblock_pattern);
							fprintf(stderr, "read_macroblock(); macroblock_intra=%d\n", macroblock_intra);
							break;
						default:
							EXIT("MpegDecoder.read_macroblock(): error. // macroblock_type");
							break;
					}

					if(macroblock_quant){
						quantizer_scale = mpegFile.read_bits_as_num(5);
						fprintf(stderr, "read_macroblock(): quantizer_scale=%d\n", quantizer_scale);
					}
				}

				if(!skipped_macroblock){
					if(macroblock_motion_forward){
						fprintf(stderr, "read_macroblock(): recon_right_for...\n");
						calc_motion_vec(forward_f, forward_r_size, full_pel_forward_vector, recon_right_for, recon_right_for_prev);
						fprintf(stderr, "read_macroblock(): recon_down_for...\n");
						calc_motion_vec(forward_f, forward_r_size, full_pel_forward_vector, recon_down_for, recon_down_for_prev);
					}
					else{
						if(picture_coding_type != 3){
							recon_right_for = 0;
							recon_down_for = 0;
						}
						else{
							recon_right_for = recon_right_for_prev;
							recon_down_for = recon_down_for_prev;
						}
					}
				}
				else{
					if(picture_coding_type == 2){
						recon_right_for = 0;
						recon_down_for = 0;
					}
					else if(picture_coding_type == 3){
						recon_right_for = recon_right_for_prev;
						recon_right_for = recon_right_for_prev;
					}
				}
				if(macroblock_intra==0)fprintf(stderr, "read_macroblock(): forward motion vector=(%d,%d)\n", recon_right_for, recon_down_for);

				if(!skipped_macroblock){
					if(macroblock_motion_backward){
						fprintf(stderr, "read_macroblock(): recon_right_back...\n");
						calc_motion_vec(backward_f, backward_r_size, full_pel_backward_vector, recon_right_back, recon_right_back_prev);
						fprintf(stderr, "read_macroblock(): recon_down_back...\n");
						calc_motion_vec(backward_f, backward_r_size, full_pel_backward_vector, recon_down_back, recon_down_back_prev);
					}
					else{
						if(picture_coding_type != 3){
							recon_right_back = 0;
							recon_down_back = 0;
						}
						else{
							recon_right_back = recon_right_back_prev;
							recon_down_back = recon_down_back_prev;
						}
					}
				}
				else{
					if(picture_coding_type == 2){
						recon_right_back = 0;
						recon_down_back = 0;
					}
					else if(picture_coding_type == 3){
						recon_right_back = recon_right_back_prev;
						recon_right_back = recon_right_back_prev;
					}
				}
				if(macroblock_intra==0)fprintf(stderr, "read_macroblock(): backward motion vector=(%d,%d)\n", recon_right_back, recon_down_back);

				if(!skipped_macroblock){
					for(int i = 0; i < 6; i++){
						pattern_code[i] = 0;
					}
					if(macroblock_pattern){
						static Huffman *coded_block_pattern_huff = NULL;
						if(!coded_block_pattern_huff){
							#include "coded_block_pattern_huff.h"
						}
						coded_block_pattern = huffman_decode(coded_block_pattern_huff);
						fprintf(stderr, "read_macroblock(): coded_block_pattern=%d\n", coded_block_pattern);
						int cbp = coded_block_pattern;
						for(int i = 0; i < 6; i++){
							if(cbp & (1 << (5 - i))){
								pattern_code[i] = 1;
							}
						}
					}
					if(macroblock_intra){
						for(int i = 0; i < 6; i++){
							pattern_code[i] = 1;
						}
					}
				}

				if(skipped_macroblock || !macroblock_intra){
					dct_dc_y_past = 128*8;
					dct_dc_cb_past = 128*8;
					dct_dc_cr_past = 128*8;
				}

				for(int i = 0; i < 6; i++){
					if(!skipped_macroblock){
						read_block(i);
						if(macroblock_intra){
							intra_block_decode(i);
						}
						else{
							if(picture_coding_type == 2){
								ref_using_motion_vec(i, recon_right_for, recon_down_for, frame_past, false);
								non_intra_block_decode(i);
							}
							if(picture_coding_type == 3){
								if(macroblock_motion_forward && !macroblock_motion_backward){
									ref_using_motion_vec(i, recon_right_for, recon_down_for, frame_past, false);
								}
								if(!macroblock_motion_forward && macroblock_motion_backward){
									ref_using_motion_vec(i, recon_right_back, recon_down_back, frame_future, false);
								}
								if(macroblock_motion_forward && macroblock_motion_backward){
									ref_using_motion_vec(i, recon_right_for, recon_down_for, frame_past, false);
									ref_using_motion_vec(i, recon_right_back, recon_down_back, frame_future, true);
								}
								non_intra_block_decode(i);
							}
						}
					}
					else{
						if(picture_coding_type == 2){
							ref_using_motion_vec(i, recon_right_for, recon_down_for, frame_past, false);
						}
						else if(picture_coding_type == 3){
							if(macroblock_motion_forward && !macroblock_motion_backward){
								ref_using_motion_vec(i, recon_right_for, recon_down_for, frame_past, false);
							}
							if(!macroblock_motion_forward && macroblock_motion_backward){
								ref_using_motion_vec(i, recon_right_back, recon_down_back, frame_future, false);
							}
							if(macroblock_motion_forward && macroblock_motion_backward){
								ref_using_motion_vec(i, recon_right_for, recon_down_for, frame_past, false);
								ref_using_motion_vec(i, recon_right_back, recon_down_back, frame_future, true);
							}
						}
					}
				}

				// D-frame
				if(picture_coding_type == 4){
					int end_of_macroblock = mpegFile.read_bits_as_num(1);
					if(end_of_macroblock != 1) EXIT("MpegDecoder.read_macroblock(): end_of_macroblock.");
				}

				previous_macroblock_address = macroblock_address;
				if(macroblock_intra) past_intra_address = macroblock_address;
				recon_right_for_prev = (picture_coding_type == 3 && macroblock_intra)? 0: recon_right_for;
				recon_down_for_prev = (picture_coding_type == 3 && macroblock_intra)? 0: recon_down_for;
				recon_right_back_prev = (picture_coding_type == 3 && macroblock_intra)? 0: recon_right_back;
				recon_down_back_prev = (picture_coding_type == 3 && macroblock_intra)? 0: recon_down_back;
			}

			return;
		}
		void calc_motion_vec(int &f, int &r_size, int &full_pel_vector, int &recon_for, int &recon_for_prev){
			static Huffman *motion_code_huff = NULL;
			if(!motion_code_huff){
				#include "motion_code_huff.h"
			}
			int motion_code;
			motion_code = huffman_decode(motion_code_huff);
			motion_code = (int)((signed char)motion_code);
			fprintf(stderr, "calc_motion_vec(): motion_code=%d\n", motion_code);
			int complement_r;
			if((f != 1) && (motion_code != 0)){
				int motion_r = mpegFile.read_bits_as_num(r_size);
				fprintf(stderr, "calc_motion_vec(): motion_r=%d\n", motion_r);
				complement_r = f - 1 - motion_r;
			}
			else{
				complement_r = 0;
			}

			int little, big;
			little = motion_code*f;
			if(little == 0){
				big = 0;
			}
			else if(little > 0){
				little -= complement_r;
				big = little - 32*f;
			}
			else{
				little += complement_r;
				big = little + 32*f;
			}
			if(little == 16*f) EXIT("read_macroblock(): error. // little");
			
			int max = 16*f - 1;
			int min = -16*f;
			int new_vector;
			new_vector = recon_for_prev + little;
			recon_for = recon_for_prev + ((new_vector <= max && new_vector >= min)? little: big);
			if(full_pel_vector){
				recon_for <<= 1;
			}
			return;
		}
	private:
		unsigned char dct_coeff_symbol(int run, int level, unsigned char symbol = 0, int *run_ret = NULL, int *level_ret = NULL){
			static int runlist[] = 
			{
				0, 1, 2, 3, 4, // level = 1
				5, 6, 7, 8, 9,
				10, 11, 12, 13, 14,
				15, 16, 17, 18, 19,
				20, 21, 22, 23, 24,
				25, 26, 27, 28, 29,
				30, 31,
				0, 1, 2, 3, 4, // level = 2
				5, 6, 7, 8, 9,
				10, 11, 12, 13, 14,
				15, 16,
				0, 1, 2, 3, 4, // level = 3
				5, 6,
				0, 1, 2, 3,    // level = 4
				0, 1, 2,       // level = 5
				0, 1,          // level = 6
				0, 1,          // level = 7
				0, 1,          // level = 8
				0, 1,          // level = 9
				0, 1,          // level = 10
				0, 1,          // level = 11
				0, 1,          // level = 12
				0, 1,          // level = 13
				0, 1,          // level = 14
				0, 1,          // level = 15
				0, 1,          // level = 16
				0, 1,          // level = 17
				0, 1,          // level = 18
	
				0, 0,          // level = 19..20
				0, 0, 0, 0, 0, // level = 21..25
				0, 0, 0, 0, 0, // level = 26..30
				0, 0, 0, 0, 0, // level = 31..35
				0, 0, 0, 0, 0, // level = 36..40

				0xFF,          // escape
				0x00           // end_of_block
			};
			static int levellist[] = 
			{
				1, 1, 1, 1, 1, // run = 0..31
				1, 1, 1, 1, 1,
				1, 1, 1, 1, 1,
				1, 1, 1, 1, 1,
				1, 1, 1, 1, 1,
				1, 1, 1, 1, 1,
				1, 1,
				2, 2, 2, 2, 2, // run = 0..16
				2, 2, 2, 2, 2, 
				2, 2, 2, 2, 2, 
				2, 2,
				3, 3, 3, 3, 3, // run = 0..6
				3, 3,
				4, 4, 4, 4,    // run = 0..3
				5, 5, 5,       // run = 0..2
				6, 6,          // run = 0..1
				7, 7,          // run = 0..1
				8, 8,          // run = 0..1
				9, 9,          // run = 0..1
				10, 10,        // run = 0..1
				11, 11,        // run = 0..1
				12, 12,        // run = 0..1
				13, 13,        // run = 0..1
				14, 14,        // run = 0..1
				15, 15,        // run = 0..1
				16, 16,        // run = 0..1
				17, 17,        // run = 0..1
				18, 18,        // run = 0..1
	
				19, 20,        // run = 0
				21, 22, 23, 24, 25,
				26, 27, 28, 29, 30,
				31, 32, 33, 34, 35,
				36, 37, 38, 39, 40,

				0xFF,          // escape
				0x00           // end_of_block
			};
			static int nSymbols = 113;
			if(run_ret == NULL || level_ret == NULL){
				//fprintf(stderr, "MpegDecoder.dct_coeff_symbol(): run=%d, level=%d\n", run, level);
				// value2symbol
				for(int i = 0; i < nSymbols; i++) if(runlist[i] == run && levellist[i] == level){
					//fprintf(stderr, "MepgDecoder.dct_coeff_symbol(): symbol=%02X\n", i);
					return (int)i;
				}
				EXIT("MpegDecoder.dct_coeff_symbol(): error. // value2symbol");
			}
			else{
				//fprintf(stderr, "MpegDecoder.dct_coeff_symbol(): symbol=%02X\n", symbol);
				// symbol2value
				if((int)symbol < 0 || (int)symbol >= nSymbols){
					EXIT("MpegDecoder.dct_coeff_symbol(): error. // symbol2value");
				}
				*run_ret = runlist[(int)symbol];
				*level_ret = levellist[(int)symbol];
				//fprintf(stderr, "MpegDecoder.dct_coeff_symbol(): run=%d, level=%d\n", run, level);
			}
			return 0;
		}
		int sign(int num){
			return (num == 0)? 0: (num > 0)? 1: -1;
		}
		int dct_dc_size_luminance;
		int dct_dc_size_chrominance;
		int dct_dc_differential;
		int dct_coeff_next;
		int dct_coeff_first;
		static const int scan[8][8];
		static const int scan_reverse[64];
		int *dct_zz;
		int dct_zz_i;
		int dct_dc_y_past;
		int dct_dc_cb_past;
		int dct_dc_cr_past;
	public:
		void read_block(int i){
			dct_zz_i = 0;
			for(int j = 0; j < 64; j++) dct_zz[j] = 0;

			if(pattern_code[i]){
				fprintf(stderr, "read_block(): block %d...\n", i);
				// dct_coeff_first
				if(macroblock_intra){
					switch(i){
						case 0: case 1: case 2: case 3:
							// for block 0..3 (Y)
							static Huffman *dct_dc_size_luminance_huff = NULL;
							if(!dct_dc_size_luminance_huff){
#include "dct_dc_size_luminance_huff.h"
							}
							dct_dc_size_luminance = huffman_decode(dct_dc_size_luminance_huff);
							fprintf(stderr, "read_block(): dct_dc_size_luminance=%d\n", dct_dc_size_luminance);
							if(dct_dc_size_luminance > 0){
								dct_dc_differential = mpegFile.read_bits_as_num(dct_dc_size_luminance);
								fprintf(stderr, "read_block(): dct_dc_differential=%d\n", dct_dc_differential);
								dct_zz[dct_zz_i++] = (dct_dc_differential&(1 << (dct_dc_size_luminance - 1)))? dct_dc_differential: (-1 << dct_dc_size_luminance)|(dct_dc_differential + 1);
								//(dct_dc_differential < pow(2, dct_dc_size_luminance - 1))? dct_dc_differential - (pow(2, dct_dc_size_luminance) - 1): dct_dc_differential;
							}
							else{
								dct_zz[dct_zz_i++] = 0;
							}
							break;
						case 4: case 5:
							// for block 4..5 (CbCr)
							static Huffman *dct_dc_size_chrominance_huff = NULL;
							if(!dct_dc_size_chrominance_huff){
#include "dct_dc_size_chrominance_huff.h"
							}
							dct_dc_size_chrominance = huffman_decode(dct_dc_size_chrominance_huff);
							fprintf(stderr, "read_block(): dct_dc_size_chrominance=%d\n", dct_dc_size_chrominance);
							if(dct_dc_size_chrominance > 0){
								dct_dc_differential = mpegFile.read_bits_as_num(dct_dc_size_chrominance);
								fprintf(stderr, "read_block(): dct_dc_differential=%d\n", dct_dc_differential);
								dct_zz[dct_zz_i++] = (dct_dc_differential&(1 << (dct_dc_size_chrominance - 1)))? dct_dc_differential: (-1 << dct_dc_size_chrominance)|(dct_dc_differential + 1);
							}
							else{
								dct_zz[dct_zz_i++] = 0;
							}
							break;
						default:
							EXIT("read_block(): error // i");
							break;
					}
				}
				else{
					// dct_coeff_first for non intra block
					static Huffman *dct_coeff_first_huff = NULL;
					if(!dct_coeff_first_huff){
						#include "dct_coeff_first_huff.h"
					}
					int run, level;
					dct_coeff_first = huffman_decode(dct_coeff_first_huff);
					dct_coeff_symbol(0, 0, (unsigned char)dct_coeff_first, &run, &level);
					// escape
					if(run == 0xFF && level == 0xFF){
						fprintf(stderr, "read_block(): dct_coeff_first=%02X, run=%d, level=%d\n", dct_coeff_first, run, level);
						fprintf(stderr, "read_block(): escape.\n");
						run = mpegFile.read_bits_as_num(6);
						level = mpegFile.read_bits_as_num(8);
						if(level == 0x80){ // b10000000...
							level = mpegFile.read_bits_as_num(8) - 256;
						}
						else if(level == 0x00){ // b00000000...
							level = mpegFile.read_bits_as_num(8);
						}
						else{
							level = (level < 128)? level: level - 256;
						}
						fprintf(stderr, "read_block(): run=%d, level=%d\n", run, level);
					}
					else{
						int sign = mpegFile.read_bits_as_num(1);
						level = sign? -level: level;
						fprintf(stderr, "read_block(): dct_coeff_first=%02X, run=%d, level=%d\n", dct_coeff_first, run, level);
					}
				}

				// dct_coeff_next
				if(picture_coding_type != 4){
					// non-dc-intra-coded (non D)
					static Huffman *dct_coeff_next_huff = NULL;
					if(!dct_coeff_next_huff){
						#include "dct_coeff_next_huff.h"
					}

					int run, level;
					while(1){
						// TODO:huffman_decode->too slow
						dct_coeff_next = huffman_decode(dct_coeff_next_huff);
						// decode run and level
						dct_coeff_symbol(0, 0, (unsigned char)dct_coeff_next, &run, &level);
						// end_of_block
						if(run == 0x00 && level == 0x00){
							fprintf(stderr, "read_block(): dct_coeff_next=%02X, run=%d, level=%d\n", dct_coeff_next, run, level);
							fprintf(stderr, "read_block(): end_of_block.\n");
							break;
						}
						// escape
						else if(run == 0xFF && level == 0xFF){
							fprintf(stderr, "read_block(): dct_coeff_next=%02X, run=%d, level=%d\n", dct_coeff_next, run, level);
							fprintf(stderr, "read_block(): escape.\n");
							run = mpegFile.read_bits_as_num(6);
							level = mpegFile.read_bits_as_num(8);
							if(level == 0x80){ // b10000000...
								level = mpegFile.read_bits_as_num(8) - 256;
							}
							else if(level == 0x00){ // b00000000...
								level = mpegFile.read_bits_as_num(8);
							}
							else{
								level = (level < 128)? level: level - 256;
							}
							fprintf(stderr, "read_block(): run=%d, level=%d\n", run, level);
						}
						// get sign bit
						else{
							int sign = mpegFile.read_bits_as_num(1);
							level = sign? -level: level;
							fprintf(stderr, "read_block(): dct_coeff_next=%02X, run=%d, level=%d\n", dct_coeff_next, run, level);
						}
						dct_zz_i += run;
						dct_zz[dct_zz_i++] = level;

					}
				}
				if(dct_zz_i > 64) EXIT("MpegDecoder.read_block(): error // block_i");
			}

			return;
		}
	private:
		void print_block(int *block, bool isZz){
			for(int i = 0; i < 8; i++){
				fprintf(stderr, "MpegDecoder.print_block(): ");
				for(int j = 0; j < 8; j++){
					int k = isZz? scan[i][j]: 8*i + j;
					fprintf(stderr, "%4d ",  *(block + k));
				}
				fprintf(stderr, "\n");
			}
			fprintf(stderr, "\n");
			return;
		}
		void idct_block(){
			double tmp[8][8];
			int r, c;
			for(r = 0; r < 8; r++) for(c = 0; c < 8; c++){
				tmp[r][c] = (double)dct_recon[r][c];
			}

			double F[8], f[8];
			for(r = 0; r < 8; r++){
				for(c = 0; c < 8; c++) F[c] = tmp[r][c];
				idct_1d(F, f);
				for(c = 0; c < 8; c++) tmp[r][c] = f[c];
			}
			for(c = 0; c < 8; c++){
				for(r = 0; r < 8; r++) F[r] = tmp[r][c];
				idct_1d(F, f);
				for(r = 0; r < 8; r++) tmp[r][c] = f[r];
			}
			for(r = 0; r < 8; r++) for(c = 0; c < 8; c++){
				dct_recon[r][c] = round(tmp[r][c]);
			}
			return;
		}
		void idct_1d(double F[8], double f[8]){
			double c[8];
			int i;
			for(i = 0; i < 8; i++) c[i] = cos((double)i*M_PI/16.0);

			double t[6];
			t[0] = F[0]*c[4];
			t[1] = F[2]*c[2];
			t[2] = F[2]*c[6];
			t[3] = F[4]*c[4];
			t[4] = F[6]*c[6];
			t[5] = F[6]*c[2];

			double a[4];
			a[0] = t[0] + t[1] + t[3] + t[4];
			a[1] = t[0] + t[2] - t[3] - t[5];
			a[2] = t[0] - t[2] - t[3] + t[5];
			a[3] = t[0] - t[1] + t[3] - t[4];

			double b[4];
			b[0] = F[1]*c[1] + F[3]*c[3] + F[5]*c[5] + F[7]*c[7];
			b[1] = F[1]*c[3] - F[3]*c[7] - F[5]*c[1] - F[7]*c[5];
			b[2] = F[1]*c[5] - F[3]*c[1] + F[5]*c[7] + F[7]*c[3];
			b[3] = F[1]*c[7] - F[3]*c[5] + F[5]*c[3] - F[7]*c[1];

			f[0] = (a[0] + b[0])/2.0;
			f[7] = (a[0] - b[0])/2.0;
			f[1] = (a[1] + b[1])/2.0;
			f[6] = (a[1] - b[1])/2.0;
			f[2] = (a[2] + b[2])/2.0;
			f[5] = (a[2] - b[2])/2.0;
			f[3] = (a[3] + b[3])/2.0;
			f[4] = (a[3] - b[3])/2.0;
			return;
		}
		int **dct_recon;
	public:
		void intra_block_decode(int i){
			for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
				dct_recon[m][n] = 2*dct_zz[scan[m][n]]*quantizer_scale*intra_quantizer_matrix[m][n]/16;
				if((dct_recon[m][n] & 1) == 0){
					dct_recon[m][n] -= sign(dct_recon[m][n]);
				}
				if(dct_recon[m][n] > 2047){
					dct_recon[m][n] = 2047;
				}
				if(dct_recon[m][n] < -2048){
					dct_recon[m][n] = -2048;
				}
			}

			switch(i){
				case 0:
					dct_recon[0][0] = dct_zz[0]*8;
					dct_recon[0][0] += (macroblock_address - past_intra_address > 1)? 128*8: dct_dc_y_past;
					dct_dc_y_past = dct_recon[0][0];
					break;
				case 1: case 2: case 3:
					dct_recon[0][0] = dct_zz[0]*8;
					dct_recon[0][0] += dct_dc_y_past;
					dct_dc_y_past = dct_recon[0][0];
					break;
				case 4:
					dct_recon[0][0] = dct_zz[0]*8;
					dct_recon[0][0] += (macroblock_address - past_intra_address > 1)? 128*8: dct_dc_cb_past;
					dct_dc_cb_past = dct_recon[0][0];
					break;
				case 5:
					dct_recon[0][0] = dct_zz[0]*8;
					dct_recon[0][0] += (macroblock_address - past_intra_address > 1)? 128*8: dct_dc_cr_past;
					dct_dc_cr_past = dct_recon[0][0];
					break;
				default:
					EXIT("MpegDecoder.read_block(): error. // dct_recon[0][0]");
					break;
			}

			// idct
			idct_block();

			// clipping
			for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
				if(dct_recon[m][n] > 255) dct_recon[m][n] = 255;
				if(dct_recon[m][n] < 0) dct_recon[m][n] = 0;
			}
			//print_block(&dct_recon[0][0], false);

			// posit block in frame
			for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
				int x, y;
				switch(i){
					case 0: case 1: case 2: case 3:
						y = mb_row*16 + (int)(i/2)*8 + m; //mb_height?????
						x = mb_column*16 + (i%2)*8 + n; //mb_width?????
						if(y >= vertical_size || x >= horizontal_size) continue;
						frame[y][x].y = dct_recon[m][n];
						break;
					case 4: case 5:
						for(int k = 0; k < 4; k++){
							y = mb_row*16 + 2*m + k/2;
							x = mb_column*16 + 2*n + k%2;
							if(y >= vertical_size || x >= horizontal_size) continue;
							if(i == 4) frame[y][x].cb = dct_recon[m][n];
							if(i == 5) frame[y][x].cr = dct_recon[m][n];
						}
						break;
					default:
						EXIT("MpegDecoder.read_block(): error.");
						break;
				}
			}
			return;
		}
		void ref_using_motion_vec(int i, int &recon_right, int &recon_down, Pixel **&frame_ref, bool isAvg){
			int right, down;
			int right_half, down_half;
			switch(i){
				case 0: case 1: case 2: case 3:
					right = recon_right >> 1;
					right_half = recon_right - 2*right;
					down = recon_down >> 1;
					down_half = recon_down - 2*down;
					for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
						int y = mb_row*16 + (int)(i/2)*8 + m;
						int x = mb_column*16 + (i%2)*8 + n;
						if(y >= vertical_size || x >= horizontal_size) continue;
						frame[y][x] = isAvg? (frame[y][x] + frame_ref[y + down][x + right])/2: frame_ref[y + down][x + right];
						if(right_half == 1) frame[y][x] += frame_ref[y + down][x + right + 1];
						if(down_half == 1) frame[y][x] += frame_ref[y + down + 1][x + right];
						if(right_half == 1 && down_half == 1) frame[y][x] += frame_ref[y + down + 1][x + right + 1];
						frame[y][x] /= (right_half == 1? 2: 1)*(down_half == 1? 2: 1);
					}
					break;
				case 4: case 5:
					right = (recon_right/2) >> 1;
					right_half = recon_right/2 - 2*right;
					down = (recon_down/2) >> 1;
					down_half = recon_down/2 - 2*down;
					for(int m = 0; m < 16; m++) for(int n = 0; n < 16; n++){
						int y = mb_row*16 + m;
						int x = mb_column*16 + n;
						if(y >= vertical_size || x >= horizontal_size) continue;
						frame[y][x] = frame_ref[y + down][x + right];
						if(right_half == 1) frame[y][x] += frame_ref[y + down][x + right + 1];
						if(down_half == 1) frame[y][x] += frame_ref[y + down + 1][x + right];
						if(right_half == 1 && down_half == 1) frame[y][x] += frame_ref[y + down + 1][x + right + 1];
						frame[y][x] /= (right_half == 1? 2: 1)*(down_half == 1? 2: 1);
					}
					break;
				default:
					EXIT("read_block(): error. //output");
					break;
			}
			return;
		}
		void non_intra_block_decode(int i){
			for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
				if(!pattern_code[i]){
					dct_recon[m][n] = 0;
					continue;
				}

				if(dct_zz[scan[m][n]] == 0){
					dct_recon[m][n] = 0;
				}
				else{
					dct_recon[m][n] = (2*dct_zz[scan[m][n]] + sign(dct_zz[scan[m][n]]))*quantizer_scale*non_intra_quantizer_matrix[m][n]/16;
					if((dct_recon[m][n] & 1) == 0){
						dct_recon[m][n] -= sign(dct_recon[m][n]);
					}
					if(dct_recon[m][n] > 2047){
						dct_recon[m][n] = 2047;
					}
					if(dct_recon[m][n] < -2048){
						dct_recon[m][n] = -2048;
					}
				}
			}

			// idct
			idct_block();

			// clipping
			for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
				if(dct_recon[m][n] > 255) dct_recon[m][n] = 255;
				if(dct_recon[m][n] < -256) dct_recon[m][n] = -256;
			}
			//print_block(&dct_recon[0][0], false);

			// add to block in frame
			for(int m = 0; m < 8; m++) for(int n = 0; n < 8; n++){
				int x, y;
				switch(i){
					case 0: case 1: case 2: case 3:
						y = mb_row*16 + (int)(i/2)*8 + m; //mb_height?????
						x = mb_column*16 + (i%2)*8 + n; //mb_width?????
						if(y >= vertical_size || x >= horizontal_size) continue;
						frame[y][x].y += dct_recon[m][n];
						if(frame[y][x].y > 255) frame[y][x].y = 255;
						if(frame[y][x].y < 0) frame[y][x].y = 0;
						break;
					case 4: case 5:
						for(int k = 0; k < 4; k++){
							y = mb_row*16 + 2*m + k/2;
							x = mb_column*16 + 2*n + k%2;
							if(y >= vertical_size || x >= horizontal_size) continue;
							if(i == 4){
								frame[y][x].cb += dct_recon[m][n];
								if(frame[y][x].cb > 255) frame[y][x].cb = 255;
								if(frame[y][x].cb < 0) frame[y][x].cb = 0;
							}
							if(i == 5){
								frame[y][x].cr += dct_recon[m][n];
								if(frame[y][x].cr > 255) frame[y][x].cr = 255;
								if(frame[y][x].cr < 0) frame[y][x].cr = 0;
							}
						}
						break;
					default:
						EXIT("MpegDecoder.read_block(): error.");
						break;
				}
			}
			return;
		}
	private:
		bool bytealigned(){
			return mpegFile.get_cur_posi_in_byte() == 0;
		}
		int nextbits(int nBits){
			//fprintf(stderr, "MpegDecoder.nextbits(): nBits=%d\n", nBits);
			int nBytes = (mpegFile.get_cur_posi_in_byte() + nBits - 1)/8 + 1;
			unsigned char buf[nBytes];
			if(!mpegFile.read(nBytes, buf)) return -1;
//			for(int i = 0; i < nBytes; i++) fprintf(stderr, "MpegDecoder.nextbits(): buf[%d]=%02X\n", i, buf[i]);
			int buf_head = mpegFile.get_cur_posi_in_byte();
			int ret = bit2num(nBits, buf, buf_head);
			mpegFile.rtrn(nBytes, buf);
			//fprintf(stderr, "MpegDecoder.nextbits(): ret=%d\n\n", ret);
			return ret;
		}
		void next_start_code(){
			while(!bytealigned()){
				//fprintf(stderr, "MpegDecoder.next_start_code(): not aligned.\n");
				int ret = mpegFile.read_bits_as_num(1);
				if(ret != 0) EXIT("MpegDecoder.next_start_code(): error. //advance != 0");
				//fprintf(stderr, "MpegDecoder.next_start_code(): advance one 0-bit.\n\n");
			}
			while(nextbits(3*8) != 0x000001){
				//fprintf(stderr, "MpegDecoder.next_start_code(): nextbits not 000001.\n");
				int ret = mpegFile.read_bits_as_num(8);
				if(ret != 0) EXIT("MpegDecoder.next_start_code(): error. //advance != 00000000");
				//fprintf(stderr, "MpegDecoder.next_start_code(): advance one 0-byte.\n\n");
			}
			return;
		}
	public:
		void read(){
			int ret;
			while((ret = nextbits(4*8)) != -1){
				switch(ret){
					case SEQUENCE_HEADER_CODE:
						fprintf(stderr, "sequence_header_code...\n\n");
						read_sequence_header();
						fprintf(stderr, "sequence_header_code done.\n\n");
						break;
					case EXTENSION_START_CODE:
						fprintf(stderr, "%d\n", ret);
						EXIT("mpeg.read() error //extension_start_code");
						break;
					case USER_DATA_START_CODE:
						EXIT("mpeg.read() error //user_data_start_code");
						break;
					case GROUP_START_CODE:
						fprintf(stderr, "group_start_code...\n\n");
						read_group_start();
						fprintf(stderr, "group_start_code done.\n\n");
						break;
					case PICTURE_START_CODE:
						fprintf(stderr, "picture_start_code...\n\n");
						read_picture_start();
						fprintf(stderr, "picture_start_code done.\n\n");
						break;
					case SEQUENCE_END_CODE:
						break;
					default:
						if(SLICE_START_CODE(ret)){
							fprintf(stderr, "slice_start_code... i=%d\n\n", frame_i);
							read_slice_start();
							fprintf(stderr, "slice_start_code done. i=%d\n\n", frame_i);
							frame_i++;
							break;
						}
						EXIT("MpegDecoder.read(): error.");
						break;
				}
			}
			fprintf(stderr, "DONE.\n");
			return;
		}
};

int main(int argc, char *argv[]){
	if(argc < 2) EXIT("Usage: main [input.m1v]");
	char *inputName = argv[1], *outputName = argv[2];

	MpegDecoder md(inputName);
	//mpegFile = fopen(inputName, "rb"); if(!jpegFile) EXIT("mpegFile error.");
	md.read();

	/*
	bmpFile = fopen(outputName, "wb"); if(!bmpFile) EXIT("bmpFile error.");
	bmp_make(jpeg_img[0], jpeg_img[1], jpeg_img[2], jpeg_frame_x, jpeg_frame_y);
	fclose(bmpFile);
	*/

	return 0;
}

/*
const int MpegDecoder::posi_in_zz[64] = {
	0, 1, 5, 6, 14, 15, 27, 28,
	2, 4, 7, 13, 16, 26, 29, 42,
	3, 8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};
*/
const int MpegDecoder::scan_reverse[64] = {
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};
const int MpegDecoder::scan[8][8] = {
	{0, 1, 5, 6, 14, 15, 27, 28},
	{2, 4, 7, 13, 16, 26, 29, 42},
	{3, 8, 12, 17, 25, 30, 41, 43},
	{9, 11, 18, 24, 31, 40, 44, 53},
	{10, 19, 23, 32, 39, 45, 52, 54},
	{20, 22, 33, 38, 46, 51, 55, 60},
	{21, 34, 37, 47, 50, 56, 59, 61},
	{35, 36, 48, 49, 57, 58, 62, 63}
};
