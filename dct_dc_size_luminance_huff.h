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

