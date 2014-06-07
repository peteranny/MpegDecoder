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

