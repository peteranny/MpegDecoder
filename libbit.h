#include <cmath>
#include <string>

#ifndef EXIT
#define EXIT(msg) {puts(msg);exit(0);}
#endif

#ifndef LIBBIT
#define LIBBIT

#define echo4bits(x) fprintf(stderr, "%s=%01X\n", (#x), (x))
#define echo8bits(x) fprintf(stderr, "%s=%02X\n", (#x), (x))
#define echo16bits(x) fprintf(stderr, "%s=%02X%02X\n", (#x), (x)[0], (x)[1])

#define echo4bitEle(x,i) fprintf(stderr, "%s%d=%01X\n", (#x), (i), (x)[i])
#define echo8bitEle(x,i) fprintf(stderr, "%s%d=%02X\n", (#x), (i), (x)[i])

/*
int byte_isSet_bit(char byte, int posi){
	char mask = 0x01 << (7 - posi);
	char bit = (byte & mask) >> (7 - posi);
	if(bit == 0x00) return 0;
	if(bit == 0x01) return 1;
	EXIT("byte_isSet_bit() error");
}
*/

/*
int bytecmp(unsigned char *byte1, unsigned char *byte2, int nByte){
	for(int i = 0; i < nByte; i++) if(byte1[i] != byte2[i]) return 0;
	return 1;
}
*/

/*
#define BITON 1
#define BITOFF 0
void set_bit(int bitType, int posi, unsigned char *buf, int nByte){
	// bit(n-1) ... bit(0)
	// byte(0)...byte(nByte - 1)
	int i = nByte - posi/8 - 1;
	int offset = posi%8;
	unsigned char mask;
	switch(bitType){
		case BITON:
			buf[i] |= 0x01 << offset;
			break;
		case BITOFF:
			buf[i] &= 0xFE << offset;
			break;
		default:
			EXIT("setBit error.");
	}
	return;
}
*/
int bit2num(int nBits, unsigned char *buf, int buf_head){
	// bit(0)...bit(7)
	// byte(0)...byte(n - 1)
	int nBytes = (buf_head + nBits - 1)/8 + 1;
	fprintf(stderr, "bit2num(): buf_head=%d, nBits=%d, nBytes=%d\n", buf_head, nBits, nBytes);
	int ret = 0;
	for(int i = 0; i < nBytes; i++){
		int head_in_byte = (i == 0)? buf_head: 0;
		int tail_in_byte = (i == nBytes - 1)? (buf_head + nBits - 1)%8: 7;
		int nBits_in_byte = tail_in_byte - head_in_byte + 1;
		unsigned char tmp = buf[i];
		tmp &= ((unsigned char)0xFF) >> head_in_byte;
		//fprintf(stderr, "bit2num(): i=%d, tmp=%02X\n", i, tmp);
		tmp &= ((unsigned char)0xFF) << (7 - tail_in_byte);
		//fprintf(stderr, "bit2num(): i=%d, tmp=%02X\n", i, tmp);
		ret = ret*pow(2, nBits_in_byte) + (int)(tmp >> (7 - tail_in_byte));
		fprintf(stderr, "bit2num(): buf[%d]=%02X, head_in_byte=%d, tail_in_byte=%d, nBits_in_byte=%d, tmp=%02X, ret=%d\n", i, buf[i], head_in_byte, tail_in_byte, nBits_in_byte, tmp, ret);
	}
	return ret;
	/*
	int byte_posi = 0;
	int bit_posi = buf_head;

	int ret = 0;
	int i, j;
	for(i = 0; i < nByte; i++){
		ret = 256*ret + (int)buf[i];
	}
	*/
}
/*
void dec2bin(int num, unsigned char *buf, int nByte){
	int i;
	for(i = 0; i < nByte; i++) buf[i] = 0;
	i = 0;
	while(num != 0){
		if(i == nByte*8) EXIT("dec2bin error.");
		int rem = num%2;
		int div = num/2;
		if(rem == 1) set_bit(BITON, i, buf, nByte);
		num = div;
		i++;
	}
	return;
}

// one bit only by far
void get_bit_range(unsigned char *src, int nByte, int start, int end, unsigned char *des){ // end: exlusive
	// bit(n - 1) ... bit(0)
	// byte(0) ... byte(nByte - 1)
	//fprintf(stderr, "get_range: start=%d, end=%d\n", start, end);
	int startByte = (nByte*8 - end)/8;
	int endByte = (nByte*8 - start - 1)/8; // inclusive
	
	int head = start; // working intervel head & tail
	int tail = (startByte == endByte)? end - 1: (nByte - endByte)*8 - 1; // tail: inclusive
	int len = end - start;
	int des_nByte = (len + 7)/8;
	int k;
	for(k = 0; k < des_nByte; k++) des[k] = 0x00;
	if(start == end) return; // fetch 0 bits
	k = des_nByte - 1; // byte of des
	// copy one byte at a time
	int i;
	for(i = endByte; i >= startByte; i--, k--){
		//fprintf(stderr, "get_range: i = %d, k = %d\n", i, k);
		unsigned char mask = 0x00;
		int j;
		for(j = head%8; j <= tail%8; j++) set_bit(BITON, j, &mask, 1);
		//fprintf(stderr, "get_range: head = %d\n", head);
		//fprintf(stderr, "get_range: tail = %d\n", tail);
		//fprintf(stderr, "get_range: mask = %02X\n", (unsigned char)mask);
		des[k] = (unsigned char)(src[i] & mask) >> head%8;
		//fprintf(stderr, "get_range: tmp = %02X\n", (unsigned char)(src[i] & mask));
		//fprintf(stderr, "get_range: des[k] = %02X\n", (unsigned char)des[k]);
		// fetch bits from the next byte to fill the byte
		if((tail - head + 1) < 8 && tail < end - 1){
			mask = 0x00;
			int lastHead = head;
			head = tail + 1;
			tail = (7 + lastHead < end)? 7 + lastHead: end - 1;
			//fprintf(stderr, "get_range: head=%d\n", head);
			//fprintf(stderr, "get_range: tail=%d\n", tail);
			for(j = head%8; j <= tail%8; j++) set_bit(BITON, j, &mask, 1);
			//fprintf(stderr, "get_range: mask = %02X\n", (unsigned char)mask);
			des[k] |= (src[i - 1] & mask) << (8 - lastHead%8);
			//fprintf(stderr, "get_range: tmp = %02X\n", (unsigned char)(src[i - 1] & mask));
			//fprintf(stderr, "get_range: des[k] = %02X\n", (unsigned char)des[k]);
		}
		if(tail == end - 1) break;
		head = tail + 1;
		tail = (head + 7 - start%8 < end)? head + 7 - start%8: end - 1;
		//fprintf(stderr, "get_range: head = %d\n", head);
		//fprintf(stderr, "get_range: tail = %d\n", tail);
	}
	return;
}

void bit_cat(unsigned char *des, unsigned char src1, unsigned char src2, int start){
	*des = (src1 << (7 - start)) | (src2 >> (start + 1));
	return;
}
*/
int parsebit(const char *bit_str){
	int ret = 0;
	for(int j = 0; j < strlen(bit_str); j++){
		int bit = bit_str[j] - '0';
		if(bit != 0 && bit != 1) EXIT("parsebit(): error.");
		ret = ret*2 + bit;
	}
	return ret;
}

void fprintb(FILE *fp, int num, int nBits){
	if(num >= pow(2, nBits)) nBits = floor(log2(num)) + 1;
	for(int i = pow(2, nBits - 1); i >= 1; i /= 2){
		if(num >= i){
			fprintf(fp, "1");
			num -= i;
		}
		else{
			fprintf(fp, "0");
		}
	}
	return;
}
/*
void show_bits(unsigned char *byte, int numBytes, int start, int end){
	int startByte = (8*numBytes - end)/8;
	int endByte = (8*numBytes - start - 1)/8;
	int i, j;
	for(i = startByte; i <= endByte; i++){
		int head = (i == endByte)? start%8: 0;
		int tail = (i == startByte)? (end - 1)%8: 7;
		for(j = tail; j >= head; j--){
			unsigned char res;
			get_bit_range(&byte[i], 1, j, j + 1, &res);
			fprintf(stderr, "%d", (int)res);
		}
		if(i < endByte) fprintf(stderr, " ");
	}
}
*/
	/*
	va_list bytes;
	va_start(bytes, num);
	fprintf(stderr, "%s = ", name);
	char str[1024];
	sfprintf(stderr, str, "%%%dX", numBits);
	int i;
	for(i = 0; i < num; i++){
		fprintf(stderr, str, (unsigned char)va_arg(bytes, char));
	}
	fprintf(stderr, "\n");
	*/

#endif
