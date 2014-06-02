int frame_i;

#include <cstdio>
#include <cstdlib>
#include "libbit.h"
#include "FileReader.h"
#include "Huffman.h"
#include "JpegDecoder.h"
//#include <string.h>
//#include "libmpeg.h"
//#include "libbmp.h"
//#include "err.h"
#ifndef EXIT
#define EXIT(msg) {fputs(msg, stderr);exit(0);}
#endif

#define PICTURE_START_CODE   0x00000100
#define SLICE_START_CODE     0x00000101
#define USER_DATA_START_CODE 0x000001B2
#define SEQUENCE_HEADER_CODE 0x000001B3
#define EXTENSION_START_CODE 0x000001B5
#define SEQUENCE_END_CODE    0x000001B7
#define GROUP_START_CODE     0x000001B8
class MpegDecoder{
	private:
		FileReader *mpegFile;
		JpegDecoder jpegDecoder;
	public:
		MpegDecoder(const char *mpeg_name){
			mpegFile = new FileReader(mpeg_name);
		}
		void read_sequence_header(){
			// sequence_header_code
			int sequence_header_code = mpegFile->read_bits_as_num(4*8);
			if(sequence_header_code != SEQUENCE_HEADER_CODE){
				EXIT("MpegDecoder.read_sequence_header(): error");
			}
			////fprintf(stderr, "MpegDecoder.read_sequence_header(): sequence_header_code=%d\n\n", sequence_header_code);

			// horizontal_size
			int horizontal_size = mpegFile->read_bits_as_num(12);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): horizontal_size=%d\n\n", horizontal_size);

			// vertical_size
			int vertical_size = mpegFile->read_bits_as_num(12);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): vertical_size=%d\n\n", vertical_size);

			// pel_aspect_ratio
			int pel_aspect_ratio = mpegFile->read_bits_as_num(4);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): pel_aspect_ratio=%d\n\n", pel_aspect_ratio);

			// picture_rate
			int picture_rate = mpegFile->read_bits_as_num(4);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): picture_rate=%d\n\n", picture_rate);

			// bit_rate
			int bit_rate = mpegFile->read_bits_as_num(18);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): bit_rate=%d\n\n", bit_rate);

			// marker_bit
			int marker_bit = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): marker_bit=%d\n\n", marker_bit);

			// vbv_buffer_size
			int vbv_buffer_size = mpegFile->read_bits_as_num(10);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): vbv_buffer_size=%d\n\n", vbv_buffer_size);

			// constrained_parameter_flag
			int constrained_parameter_flag = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): constrained_parameter_flag=%d\n\n", constrained_parameter_flag);

			// load_intra_quantizer_matrix
			int load_intra_quantizer_matrix = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): load_intra_quantizer_matrix=%d\n\n", load_intra_quantizer_matrix);
			
			if(load_intra_quantizer_matrix){
				// intra_quantizer_matrix
				int intra_quantizer_matrix[64];
				for(int i = 0; i < 64; i++){
					intra_quantizer_matrix[i] = mpegFile->read_bits_as_num(8);
					//fprintf(stderr, "MpegDecoder.read_sequence_header(): intra_quantizer_matrix[%d]=%d\n\n", i, intra_quantizer_matrix[i]);
				}
			}

			// load_non_intra_quantizer_matrix
			int load_non_intra_quantizer_matrix = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_sequence_header(): load_non_intra_quantizer_matrix=%d\n\n", load_non_intra_quantizer_matrix);

			if(load_non_intra_quantizer_matrix){
				// non_intra_quantizer_matrix
				int non_intra_quantizer_matrix[64];
				for(int i = 0; i < 64; i++){
					non_intra_quantizer_matrix[i] = mpegFile->read_bits_as_num(8);
					//fprintf(stderr, "MpegDecoder.read_sequence_header(): non_intra_quantizer_matrix[%d]=%d\n\n", i, non_intra_quantizer_matrix[i]);
				}
			}

			next_start_code();

			return;
		}
		void read_group_start(){
			// group_start_code
			int group_start_code = mpegFile->read_bits_as_num(4*8);
			if(group_start_code != GROUP_START_CODE){
				EXIT("MpegDecoder.read_group_start(): error");
			}
			//fprintf(stderr, "MpegDecoder.read_group_start(): group_start_code=%d\n\n", group_start_code);

			// time_code
			// drop_frame_flag
			int drop_frame_flag = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_group_start(): drop_frame_flag=%d\n\n", drop_frame_flag);
			// time_code_hours
			int time_code_hours = mpegFile->read_bits_as_num(5);
			//fprintf(stderr, "MpegDecoder.read_group_start(): time_code_hours=%d\n\n", time_code_hours);
			// time_code_minutes
			int time_code_minutes = mpegFile->read_bits_as_num(6);
			//fprintf(stderr, "MpegDecoder.read_group_start(): time_code_minutes=%d\n\n", time_code_minutes);
			// marker_bit
			int marker_bit = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_group_start(): marker_bit=%d\n\n", marker_bit);
			// time_code_seconds
			int time_code_seconds = mpegFile->read_bits_as_num(6);
			//fprintf(stderr, "MpegDecoder.read_group_start(): time_code_seconds=%d\n\n", time_code_seconds);
			// time_code_pictures
			int time_code_pictures = mpegFile->read_bits_as_num(6);
			//fprintf(stderr, "MpegDecoder.read_group_start(): time_code_pictures=%d\n\n", time_code_pictures);
		
			// closed_gop
			int closed_gop = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_group_start(): closed_gop=%d\n\n", closed_gop);
			
			// broken_link
			int broken_link = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_group_start(): broken_link=%d\n\n", broken_link);
			
			next_start_code();

			return;
		}
	private:
		int picture_coding_type;
		int forward_f_code;
		int forward_r_size;
		int forward_f;
	public:
		void read_picture_start(){
			// picture_start_code
			int picture_start_code = mpegFile->read_bits_as_num(4*8);
			if(picture_start_code != PICTURE_START_CODE){
				EXIT("MpegDecoder.read_picture_start(): error");
			}
			//fprintf(stderr, "MpegDecoder.read_picture_start(): picture_start_code=%d\n\n", picture_start_code);

			// temporal_reference
			int temporal_reference = mpegFile->read_bits_as_num(10);
			//fprintf(stderr, "MpegDecoder.read_picture_start(): temporal_reference=%d\n\n", temporal_reference);

			// picture_coding_type
			picture_coding_type = mpegFile->read_bits_as_num(3);
			//fprintf(stderr, "MpegDecoder.read_picture_start(): picture_coding_type=%d\n\n", picture_coding_type);

			// vbv_delay
			int vbv_delay = mpegFile->read_bits_as_num(16);
			//fprintf(stderr, "MpegDecoder.read_picture_start(): vbv_delay=%d\n\n", vbv_delay);

			if(picture_coding_type == 2 || picture_coding_type == 3){
				// full_pel_forward_vector
				int full_pel_forward_vector = mpegFile->read_bits_as_num(1);
				//fprintf(stderr, "MpegDecoder.read_picture_start(): full_pel_forward_vector=%d\n\n", full_pel_forward_vector);

				// forward_f_code
				forward_f_code = mpegFile->read_bits_as_num(3);
				EXIT("MpegDecoder.read_picture_start(): error. // forward_f_codd=0");
				forward_r_size = forward_f_code - 1;
				forward_f = 1 << forward_r_size;
				//fprintf(stderr, "MpegDecoder.read_picture_start(): forward_f_code=%d\n\n", forward_f_code);
				//fprintf(stderr, "MpegDecoder.read_picture_start(): forward_r_size=%d\n\n", forward_r_size);
				//fprintf(stderr, "MpegDecoder.read_picture_start(): forward_f=%d\n\n", forward_f);
			}

			if(picture_coding_type == 3){
				// full_pel_backward_vector
				int full_pel_backward_vector = mpegFile->read_bits_as_num(1);
				//fprintf(stderr, "MpegDecoder.read_picture_start(): full_pel_backward_vector=%d\n\n", full_pel_backward_vector);

				// backward_f_code
				int backward_f_code = mpegFile->read_bits_as_num(3);
				//fprintf(stderr, "MpegDecoder.read_picture_start(): backward_f_code=%d\n\n", backward_f_code);
			}

			while(nextbits(1) == 1){ // b1
				// extra_bit_picture
				int extra_bit_picture = mpegFile->read_bits_as_num(1);
				//fprintf(stderr, "MpegDecoder.read_picture_start(): extra_bit_picture=%d\n\n", extra_bit_picture);
				
				// extra_information_picture
				int extra_information_picture = mpegFile->read_bits_as_num(8);
				//fprintf(stderr, "MpegDecoder.read_picture_start(): extra_information_picture=%d\n\n", extra_information_picture);
			}

			// extra_bit_picture
			int extra_bit_picture = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_picture_start(): extra_bit_picture=%d\n\n", extra_bit_picture);
	
			next_start_code();
			
			return;
		}
		void read_slice_start(){
			// slice_start_code
			int slice_start_code = mpegFile->read_bits_as_num(4*8);
			if(slice_start_code != SLICE_START_CODE){
				EXIT("MpegDecoder.read_slice_start(): error");
			}
			//fprintf(stderr, "MpegDecoder.read_slice_start(): slice_start_code=%d\n\n", slice_start_code);
			
			// quantizer_scale
			int quantizer_scale = mpegFile->read_bits_as_num(5);
			//fprintf(stderr, "MpegDecoder.read_slice_start(): quantizer_scale=%d\n\n", quantizer_scale);

			while(nextbits(1) == 1){ // b1
				// extra_bit_slice
				int extra_bit_slice = mpegFile->read_bits_as_num(1);
				//fprintf(stderr, "MpegDecoder.read_slice_start(): extra_bit_slice=%d\n\n", extra_bit_slice);
			
				// extra_information_slice
				int extra_information_slice = mpegFile->read_bits_as_num(8);
				//fprintf(stderr, "MpegDecoder.read_slice_start(): extra_information_slice=%d\n\n", extra_information_slice);
			}

			// extra_bit_slice
			int extra_bit_slice = mpegFile->read_bits_as_num(1);
			//fprintf(stderr, "MpegDecoder.read_slice_start(): extra_bit_slice=%d\n\n", extra_bit_slice);

			int i = 1;
			while(nextbits(23) != 0){ // b00000000000000000000000
				//fprintf(stderr, "MpegDecoder.read_slice_start(): read_macroblock... i=%d\n\n", i);
				read_macroblock();
				//fprintf(stderr, "MpegDecoder.read_slice_start(): read_macroblock done. i=%d\n\n", i++);
			}
			next_start_code();

			return;
		}
	private:
		int macroblock_type;
		int macroblock_quant;
		int macroblock_motion_forward;
		int macroblock_motion_backward;
		int macroblock_pattern;
		int macroblock_intra;
		int pattern_code[6];
		int huffman_decode(Huffman *h, int nBits){
			int ret = h->decode(nextbits(nBits));
			mpegFile->read_bits_as_num(h->get_codelen(ret));
			return ret;
		}
	public:
		void read_macroblock(){
			while(true){
				// macroblock_address_increment
				static Huffman *macroblock_address_increment_huff = NULL;
				if(!macroblock_address_increment_huff){
					macroblock_address_increment_huff = new Huffman(
						false,
						(int[]){
							0, 1, 0, 2, 2, 2, 0, 2, 6, 0, 6, 14
						},
						11,
						(unsigned char[]){
							1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
							11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
							21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
							31, 32, 33,
							0x00, // macroblock_stuffing
							0xFF // macroblock_escape 
						},
						35
					);
					macroblock_address_increment_huff->make_codewords();
					macroblock_address_increment_huff->set_codeword(0x00, "00000001111");
					macroblock_address_increment_huff->set_codeword(0xFF, "00000001000");
					macroblock_address_increment_huff->make_hash_table();
				}
				int macroblock_address_increment = huffman_decode(macroblock_address_increment_huff, 11);
				if(macroblock_address_increment == 0x00){
					//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_stuffing.");
					continue;
				}
				if(macroblock_address_increment == 0xFF){
					//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_escape.");
					continue;
				}
				//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_address_increment=%d\n\n", macroblock_address_increment);
				break;
			}

			// macroblock_type
			switch(picture_coding_type){
				case 1: // intra-coded (I)
					static Huffman *macroblock_type_I_huff = NULL;
					if(!macroblock_type_I_huff){
						macroblock_type_I_huff = new Huffman(
							false,
							(int[]){
								0, 1, 1
							},
							2,
							(unsigned char[]){
								1, 2
							},
							2
						);
						macroblock_type_I_huff->make_codewords();
						macroblock_type_I_huff->make_hash_table();
					}
					macroblock_type = huffman_decode(macroblock_type_I_huff, 2);
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
				default:
					EXIT("MpegDecoder.read_macroblock(): error. // macroblock_type");
					break;
			}
			//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_type=%d\n", macroblock_type);
			//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_quant=%d\n", macroblock_quant);
			//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_motion_forward=%d\n", macroblock_motion_forward);
			//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_motion_backward=%d\n", macroblock_motion_backward);
			//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_pattern=%d\n", macroblock_pattern);
			//fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_intra=%d\n", macroblock_intra);
			//fprintf(stderr, "\n");

			int quantizer_scale;
			if(macroblock_quant){
				quantizer_scale = mpegFile->read_bits_as_num(5);
				//fprintf(stderr, "MpegDecoder.read_macroblock(): quantizer_scale=%d\n\n", quantizer_scale);
			}

			int motion_horizontal_forward_code;
			if(macroblock_motion_forward){
				EXIT("MpegDecoder.read_macroblock(): error. // motion_horizontal_forward_code");
			}

			int motion_horizontal_backward_code;
			if(macroblock_motion_backward){
				EXIT("MpegDecoder.read_macroblock(): error. // motion_horizontal_backward_code");
			}

			int coded_block_pattern;
			for(int i = 0; i < 6; i++){
				pattern_code[i] = 0;
			}
			if(macroblock_pattern){
				EXIT("MpegDecoder.read_macroblock(): error. // coded_block_pattern");
				for(int i = 0; i < 6; i++){
				}
			}
			if(macroblock_intra){
				for(int i = 0; i < 6; i++){
					pattern_code[i] = 1;
				}
			}

			for(int i = 0; i < 6; i++){
				//fprintf(stderr, "MpegDecoder.read_macroblock(): read_block(%d)...\n\n", i);
				read_block(i);
				//fprintf(stderr, "MpegDecoder.read_macroblock(): read_block(%d) done.\n\n", i);
			}

			if(picture_coding_type == 4){
				int end_of_macroblock = mpegFile->read_bits_as_num(1);
				if(end_of_macroblock != 1) EXIT("MpegDecoder.read_macroblock(): end_of_macroblock.");
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
	public:
		void read_block(int i){
			// shared by dct_coeff_first and dct_coeff_next
			static Huffman *dct_coeff_huff = NULL;
			if(!dct_coeff_huff){
				dct_coeff_huff = new Huffman(
					true,
					(int[]){
						0, 
					},
					0,
					(unsigned char[]){
					},
					0
				);
				dct_coeff_huff->make_codewords();
				# include "dct_coeff_huff_set.h"
				dct_coeff_huff->make_hash_table();
			}
	
			if(pattern_code[i]){
				if(macroblock_intra){
					if(i < 4){
						static Huffman *dct_dc_size_luminance_huff = NULL;
						if(!dct_dc_size_luminance_huff){
							dct_dc_size_luminance_huff = new Huffman(
								true,
								(int[]){
									0, 0, 2, 3, 1, 1, 1, 1
								},
								7,
								(unsigned char[]){
									1, 2, 0, 3, 4, 5, 6, 7, 8
								},
								9
							);
							dct_dc_size_luminance_huff->make_codewords();
							dct_dc_size_luminance_huff->make_hash_table();
						}
						int dct_dc_size_luminance = huffman_decode(dct_dc_size_luminance_huff, 7);
						//fprintf(stderr, "MpegDecoder.read_macroblock(): dct_dc_size_luminance=%d\n\n", dct_dc_size_luminance);
						if(dct_dc_size_luminance > 0){
							int dct_dc_differential_luminance = mpegFile->read_bits_as_num(dct_dc_size_luminance);
							//fprintf(stderr, "MpegDecoder.read_macroblock(): dct_dc_differential_luminance=%d\n\n", dct_dc_differential_luminance);
						}
						// dct_zz
					}
					else{
						static Huffman *dct_dc_size_chrominance_huff = NULL;
						if(!dct_dc_size_chrominance_huff){
							dct_dc_size_chrominance_huff = new Huffman(
								true,
								(int[]){
									0, 0, 3, 1, 1, 1, 1, 1, 1
								},
								8,
								(unsigned char[]){
									0, 1, 2, 3, 4, 5, 6, 7, 8
								},
								9
							);
							dct_dc_size_chrominance_huff->make_codewords();
							dct_dc_size_chrominance_huff->make_hash_table();
						}
						int dct_dc_size_chrominance = huffman_decode(dct_dc_size_chrominance_huff, 8);
						//fprintf(stderr, "MpegDecoder.read_macroblock(): dct_dc_size_chrominance=%d\n\n", dct_dc_size_chrominance);
						if(dct_dc_size_chrominance > 0){
							int dct_dc_differential_chrominance = mpegFile->read_bits_as_num(dct_dc_size_chrominance);
							//fprintf(stderr, "MpegDecoder.read_macroblock(): dct_dc_differential_chrominance=%d\n\n", dct_dc_differential_chrominance);
						}
						// dct_zz
					}
				}
				else{
					// TODO
					dct_coeff_huff->set_codeword(dct_coeff_symbol(0, 1), "1");
					dct_coeff_huff->make_hash_table();
					EXIT("MpegDecoder.read_macroblock(): error. // dct_coeff_first");
				}

				// dct_coeff_next
				if(picture_coding_type != 4){
					// due to the difference from dct_coeff_first
					dct_coeff_huff->set_codeword(dct_coeff_symbol(0, 1), "11");
					dct_coeff_huff->make_hash_table();
					int run, level;
					while(1){
						unsigned char dct_coeff_next = huffman_decode(dct_coeff_huff, 16);
						// decode run and level
						dct_coeff_symbol(0, 0, dct_coeff_next, &run, &level);
						// end_of_block
						if(run == 0x00 && level == 0x00){
							//fprintf(stderr, "MpegDecoder.read_macroblock(): dct_coeff_next=%02X, run=%d, level=%d\n", dct_coeff_next, run, level);
							//fprintf(stderr, "MpegDecoder.read_macroblock(): end_of_block.\n\n");
							break;
						}
						// escape
						else if(run == 0xFF && level == 0xFF){
							//fprintf(stderr, "MpegDecoder.read_macroblock(): dct_coeff_next=%02X, run=%d, level=%d\n", dct_coeff_next, run, level);
							//fprintf(stderr, "MpegDecoder.read_macroblock(): escape.\n\n");
							run = mpegFile->read_bits_as_num(6);
							level = mpegFile->read_bits_as_num(8);
							if(level == 0x80){ // b10000000...
								level = mpegFile->read_bits_as_num(8) - 256;
							}
							else if(level == 0x00){ // b00000000...
								level = mpegFile->read_bits_as_num(8);
							}
							else{
								level = (level < 128)? level: level - 256;
							}
							//fprintf(stderr, "MpegDecoder.read_macroblock(): run=%d, level=%d\n\n", run, level);
						}
						// get sign bit
						else{
							int sign = mpegFile->read_bits_as_num(1);
							level = sign? -level: level;
							//fprintf(stderr, "MpegDecoder.read_macroblock(): dct_coeff_next=%02X, run=%d, level=%d\n\n", dct_coeff_next, run, level);
						}
					}
				}
			}
		}
	private:
		bool bytealigned(){
			return mpegFile->get_cur_posi_in_byte() == 0;
		}
		int nextbits(int nBits){
			//fprintf(stderr, "MpegDecoder.nextbits(): nBits=%d\n", nBits);
			int nBytes = (mpegFile->get_cur_posi_in_byte() + nBits - 1)/8 + 1;
			unsigned char buf[nBytes];
			if(!mpegFile->read(nBytes, buf)) return -1;
			//for(int i = 0; i < nBytes; i++) fprintf(stderr, "MpegDecoder.nextbits(): buf[%d]=%02X\n", i, buf[i]);
			int buf_head = mpegFile->get_cur_posi_in_byte();
			int ret = bit2num(nBits, buf, buf_head);
			mpegFile->rtrn(nBytes, buf);
			//fprintf(stderr, "MpegDecoder.nextbits(): ret=%d\n\n", ret);
			return ret;
		}
		void next_start_code(){
			while(!bytealigned()){
				//fprintf(stderr, "MpegDecoder.next_start_code(): not aligned.\n");
				int ret = mpegFile->read_bits_as_num(1);
				if(ret != 0) EXIT("MpegDecoder.next_start_code(): error. //advance != 0");
				//fprintf(stderr, "MpegDecoder.next_start_code(): advance one 0-bit.\n\n");
			}
			while(nextbits(3*8) != 0x000001){
				//fprintf(stderr, "MpegDecoder.next_start_code(): nextbits not 000001.\n");
				int ret = mpegFile->read_bits_as_num(8);
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
						//fprintf(stderr, "sequence_header_code...\n\n");
						read_sequence_header();
						//fprintf(stderr, "sequence_header_code done.\n\n");
						break;
					case EXTENSION_START_CODE:
						//fprintf(stderr, "%d\n", ret);
						EXIT("mpeg.read() error //extension_start_code");
						break;
					case USER_DATA_START_CODE:
						EXIT("mpeg.read() error //user_data_start_code");
						break;
					case GROUP_START_CODE:
						//fprintf(stderr, "group_start_code...\n\n");
						read_group_start();
						//fprintf(stderr, "group_start_code done.\n\n");
						break;
					case PICTURE_START_CODE:
						//fprintf(stderr, "picture_start_code...\n\n");
						read_picture_start();
						//fprintf(stderr, "picture_start_code done.\n\n");
						break;
					case SLICE_START_CODE:
						//fprintf(stderr, "slice_start_code... i=%d\n\n", i);
						read_slice_start();
						fprintf(stderr, "slice_start_code done. i=%d\n\n", frame_i++);
						break;
					case SEQUENCE_END_CODE:
						break;
					default:
						EXIT("mpeg.read() error");
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
