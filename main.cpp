#include <cstdio>
#include <cstdlib>
#include "libbit.h"
#include "FileReader.h"
#include "Huffman.h"
//#include <string.h>
//#include "libmpeg.h"
//#include "libbmp.h"
//#include "err.h"
#ifndef EXIT
#define EXIT(msg) {puts(msg);exit(0);}
#endif

#define PICTURE_START_CODE   0x00000100
#define SLICE_START_CODE     0x00000101
#define USER_DATA_START_CODE 0x000001B2
#define SEQUENCE_HEADER_CODE 0x000001B3
#define EXTENSION_START_CODE 0x000001B5
#define GROUP_START_CODE     0x000001B8
class MpegDecoder{
	private:
		FileReader *mpegFile;
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
			fprintf(stderr, "MpegDecoder.read_sequence_header(): sequence_header_code=%d\n\n", sequence_header_code);

			// horizontal_size
			int horizontal_size = mpegFile->read_bits_as_num(12);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): horizontal_size=%d\n\n", horizontal_size);

			// vertical_size
			int vertical_size = mpegFile->read_bits_as_num(12);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): vertical_size=%d\n\n", vertical_size);

			// pel_aspect_ratio
			int pel_aspect_ratio = mpegFile->read_bits_as_num(4);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): pel_aspect_ratio=%d\n\n", pel_aspect_ratio);

			// picture_rate
			int picture_rate = mpegFile->read_bits_as_num(4);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): picture_rate=%d\n\n", picture_rate);

			// bit_rate
			int bit_rate = mpegFile->read_bits_as_num(18);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): bit_rate=%d\n\n", bit_rate);

			// marker_bit
			int marker_bit = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): marker_bit=%d\n\n", marker_bit);

			// vbv_buffer_size
			int vbv_buffer_size = mpegFile->read_bits_as_num(10);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): vbv_buffer_size=%d\n\n", vbv_buffer_size);

			// constrained_parameter_flag
			int constrained_parameter_flag = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): constrained_parameter_flag=%d\n\n", constrained_parameter_flag);

			// load_intra_quantizer_matrix
			int load_intra_quantizer_matrix = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): load_intra_quantizer_matrix=%d\n\n", load_intra_quantizer_matrix);
			
			if(load_intra_quantizer_matrix){
				// intra_quantizer_matrix
				int intra_quantizer_matrix[64];
				for(int i = 0; i < 64; i++){
					intra_quantizer_matrix[i] = mpegFile->read_bits_as_num(8);
					fprintf(stderr, "MpegDecoder.read_sequence_header(): intra_quantizer_matrix[%d]=%d\n\n", i, intra_quantizer_matrix[i]);
				}
			}

			// load_non_intra_quantizer_matrix
			int load_non_intra_quantizer_matrix = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_sequence_header(): load_non_intra_quantizer_matrix=%d\n\n", load_non_intra_quantizer_matrix);

			if(load_non_intra_quantizer_matrix){
				// non_intra_quantizer_matrix
				int non_intra_quantizer_matrix[64];
				for(int i = 0; i < 64; i++){
					non_intra_quantizer_matrix[i] = mpegFile->read_bits_as_num(8);
					fprintf(stderr, "MpegDecoder.read_sequence_header(): non_intra_quantizer_matrix[%d]=%d\n\n", i, non_intra_quantizer_matrix[i]);
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
			fprintf(stderr, "MpegDecoder.read_group_start(): group_start_code=%d\n\n", group_start_code);

			// time_code
			// drop_frame_flag
			int drop_frame_flag = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_group_start(): drop_frame_flag=%d\n\n", drop_frame_flag);
			// time_code_hours
			int time_code_hours = mpegFile->read_bits_as_num(5);
			fprintf(stderr, "MpegDecoder.read_group_start(): time_code_hours=%d\n\n", time_code_hours);
			// time_code_minutes
			int time_code_minutes = mpegFile->read_bits_as_num(6);
			fprintf(stderr, "MpegDecoder.read_group_start(): time_code_minutes=%d\n\n", time_code_minutes);
			// marker_bit
			int marker_bit = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_group_start(): marker_bit=%d\n\n", marker_bit);
			// time_code_seconds
			int time_code_seconds = mpegFile->read_bits_as_num(6);
			fprintf(stderr, "MpegDecoder.read_group_start(): time_code_seconds=%d\n\n", time_code_seconds);
			// time_code_pictures
			int time_code_pictures = mpegFile->read_bits_as_num(6);
			fprintf(stderr, "MpegDecoder.read_group_start(): time_code_pictures=%d\n\n", time_code_pictures);
		
			// closed_gop
			int closed_gop = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_group_start(): closed_gop=%d\n\n", closed_gop);
			
			// broken_link
			int broken_link = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_group_start(): broken_link=%d\n\n", broken_link);
			
			next_start_code();

			return;
		}
		void read_picture_start(){
			// picture_start_code
			int picture_start_code = mpegFile->read_bits_as_num(4*8);
			if(picture_start_code != PICTURE_START_CODE){
				EXIT("MpegDecoder.read_picture_start(): error");
			}
			fprintf(stderr, "MpegDecoder.read_picture_start(): picture_start_code=%d\n\n", picture_start_code);

			// temporal_reference
			int temporal_reference = mpegFile->read_bits_as_num(10);
			fprintf(stderr, "MpegDecoder.read_picture_start(): temporal_reference=%d\n\n", temporal_reference);

			// picture_coding_type
			int picture_coding_type = mpegFile->read_bits_as_num(3);
			fprintf(stderr, "MpegDecoder.read_picture_start(): picture_coding_type=%d\n\n", picture_coding_type);

			// vbv_delay
			int vbv_delay = mpegFile->read_bits_as_num(16);
			fprintf(stderr, "MpegDecoder.read_picture_start(): vbv_delay=%d\n\n", vbv_delay);

			if(picture_coding_type == 2 || picture_coding_type == 3){
				// full_pel_forward_vector
				int full_pel_forward_vector = mpegFile->read_bits_as_num(1);
				fprintf(stderr, "MpegDecoder.read_picture_start(): full_pel_forward_vector=%d\n\n", full_pel_forward_vector);

				// forward_f_code
				int forward_f_code = mpegFile->read_bits_as_num(3);
				fprintf(stderr, "MpegDecoder.read_picture_start(): forward_f_code=%d\n\n", forward_f_code);
			}

			if(picture_coding_type == 3){
				// full_pel_backward_vector
				int full_pel_backward_vector = mpegFile->read_bits_as_num(1);
				fprintf(stderr, "MpegDecoder.read_picture_start(): full_pel_backward_vector=%d\n\n", full_pel_backward_vector);

				// backward_f_code
				int backward_f_code = mpegFile->read_bits_as_num(3);
				fprintf(stderr, "MpegDecoder.read_picture_start(): backward_f_code=%d\n\n", backward_f_code);
			}

			while(nextbits(1) == 1){ // b1
				// extra_bit_picture
				int extra_bit_picture = mpegFile->read_bits_as_num(1);
				fprintf(stderr, "MpegDecoder.read_picture_start(): extra_bit_picture=%d\n\n", extra_bit_picture);
				
				// extra_information_picture
				int extra_information_picture = mpegFile->read_bits_as_num(8);
				fprintf(stderr, "MpegDecoder.read_picture_start(): extra_information_picture=%d\n\n", extra_information_picture);
			}

			// extra_bit_picture
			int extra_bit_picture = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_picture_start(): extra_bit_picture=%d\n\n", extra_bit_picture);
	
			next_start_code();
			
			return;
		}
		void read_slice_start(){
			// slice_start_code
			int slice_start_code = mpegFile->read_bits_as_num(4*8);
			if(slice_start_code != SLICE_START_CODE){
				EXIT("MpegDecoder.read_slice_start(): error");
			}
			fprintf(stderr, "MpegDecoder.read_slice_start(): slice_start_code=%d\n\n", slice_start_code);
			
			// quantizer_scale
			int quantizer_scale = mpegFile->read_bits_as_num(5);
			fprintf(stderr, "MpegDecoder.read_slice_start(): quantizer_scale=%d\n\n", quantizer_scale);

			while(nextbits(1) == 1){ // b1
				// extra_bit_slice
				int extra_bit_slice = mpegFile->read_bits_as_num(1);
				fprintf(stderr, "MpegDecoder.read_slice_start(): extra_bit_slice=%d\n\n", extra_bit_slice);
			
				// extra_information_slice
				int extra_information_slice = mpegFile->read_bits_as_num(8);
				fprintf(stderr, "MpegDecoder.read_slice_start(): extra_information_slice=%d\n\n", extra_information_slice);
			}

			// extra_bit_slice
			int extra_bit_slice = mpegFile->read_bits_as_num(1);
			fprintf(stderr, "MpegDecoder.read_slice_start(): extra_bit_slice=%d\n\n", extra_bit_slice);

			while(nextbits(23) != 0){ // b00000000000000000000000
				fprintf(stderr, "MpegDecoder.read_slice_start(): read_macroblock...\n\n");
				read_macroblock();
				fprintf(stderr, "MpegDecoder.read_slice_start(): read_macroblock done.\n\n");
			}
			next_start_code();

			return;
		}
		void read_macroblock(){

			while(nextbits(11) == 15){ // b00000001111
				// macroblock_stuffing
				int macroblock_stuffing = mpegFile->read_bits_as_num(11);
				fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_stuffing=%d\n\n", macroblock_stuffing);
			}

			while(nextbits(11) == 8){ // b00000001000
				// macroblock_escape
				int macroblock_escape = mpegFile->read_bits_as_num(11);
				fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_escape=%d\n\n", macroblock_escape);
			}

			// variable length huffman
			Huffman *h = new Huffman(
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
					0, // macroblock_stuffing
					255 // macroblock_escape 
					},
					35
					);
			h->make_codewords();
			h->set_codeword(0, "00000001111");
			h->set_codeword(255, "00000001000");
			h->make_hash_table();

			// macroblock_address_increment
			int macroblock_address_increment = h->decode(nextbits(11));
			mpegFile->read_bits_as_num(h->get_codelen(macroblock_address_increment));
			fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_address_increment=%d\n\n", macroblock_address_increment);
		
			exit(0);
			// macroblock_type
			// TODO: another huffman
			int macroblock_type = h->decode(nextbits(11));
			mpegFile->read_bits_as_num(h->get_codelen(macroblock_type));
			fprintf(stderr, "MpegDecoder.read_macroblock(): macroblock_type=%d\n\n", macroblock_type);
			
			exit(0);
			return;
		}
	private:
		bool bytealigned(){
			return mpegFile->get_cur_posi_in_byte() == 0;
		}
		int nextbits(int nBits){
			fprintf(stderr, "MpegDecoder.nextbits(): nBits=%d\n", nBits);
			int nBytes = (mpegFile->get_cur_posi_in_byte() + nBits - 1)/8 + 1;
			unsigned char buf[nBytes];
			mpegFile->read(nBytes, buf);
			for(int i = 0; i < nBytes; i++) fprintf(stderr, "MpegDecoder.nextbits(): buf[%d]=%02X\n", i, buf[i]);
			int buf_head = mpegFile->get_cur_posi_in_byte();
			int ret = bit2num(nBits, buf, buf_head);
			mpegFile->rtrn(nBytes, buf);
			fprintf(stderr, "MpegDecoder.nextbits(): ret=%d\n\n", ret);
			return ret;
		}
		void next_start_code(){
			while(!bytealigned()){
				fprintf(stderr, "MpegDecoder.next_start_code(): not aligned.\n");
				int ret = mpegFile->read_bits_as_num(1);
				if(ret != 0) EXIT("MpegDecoder.next_start_code(): error. //advance != 0");
				fprintf(stderr, "MpegDecoder.next_start_code(): advance one 0-bit.\n\n");
			}
			while(nextbits(3*8) != 0x000001){
				fprintf(stderr, "MpegDecoder.next_start_code(): nextbits not 000001.\n");
				int ret = mpegFile->read_bits_as_num(8);
				if(ret != 0) EXIT("MpegDecoder.next_start_code(): error. //advance != 00000000");
				fprintf(stderr, "MpegDecoder.next_start_code(): advance one 0-byte.\n\n");
			}
			return;
		}
	public:
		void read(){
			int ret;
			while((ret = nextbits(4*8))){
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
					case SLICE_START_CODE:
						fprintf(stderr, "slice_start_code...\n\n");
						read_slice_start();
						fprintf(stderr, "slice_start_code done.\n\n");
						exit(0);
						break;
					default:
						EXIT("mpeg.read() error");
						break;
				}
			}
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
