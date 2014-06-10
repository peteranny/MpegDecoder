motion_code_huff = new Huffman(
	false,
	(int[]){
		0, 1, 0, 2, 2, 2, 0, 2, 6, 0, 6, 12
	},
	11,
	(unsigned char[]){
		0,
		(unsigned char)(-1), 1,
		(unsigned char)(-2), 2,
		(unsigned char)(-3), 3,
		(unsigned char)(-4), 4,
		(unsigned char)(-5), 5,
		(unsigned char)(-6), 6,
		(unsigned char)(-7), 7,
		(unsigned char)(-8), 8,
		(unsigned char)(-9), 9,
		(unsigned char)(-10), 10,
		(unsigned char)(-11), 11,
		(unsigned char)(-12), 12,
		(unsigned char)(-13), 13,
		(unsigned char)(-14), 14,
		(unsigned char)(-15), 15,
		(unsigned char)(-16), 16,
	},
	33
);
motion_code_huff->make_codewords();
motion_code_huff->make_hash_table();

