#include <cstring>

#ifndef HUFFMAN
#define HUFFMAN
class Huffman{
	private:
		int *nCodesOfLen;
		int maxLen;

		int *codelen; // symbol index -> symbol codelen
		unsigned char *symbol; // symbol index -> symbol
		int nSymbol;

		int *codeword; // symbol index -> symbol codeword
		int nCodewords;
		bool isIncr;

		unsigned char *hash; // codeword -> symbol
	public:
		Huffman(bool isIncr, int nCodesOfLen[], int maxLen, unsigned char symbol[], int nSymbol){
			this->isIncr = isIncr;
			this->maxLen = maxLen;
			this->nCodesOfLen = new int[maxLen + 1];
			memcpy(this->nCodesOfLen, nCodesOfLen, sizeof(int)*(maxLen + 1));
			this->nSymbol = nSymbol;
			this->symbol = new unsigned char[nSymbol];
			this->codelen = new int[nSymbol];
			memcpy(this->symbol, symbol, sizeof(unsigned char)*(nSymbol));
			this->nCodewords = pow(2, maxLen);
			this->hash = new unsigned char[nCodewords];
			this->codeword = new int[nSymbol + 1];
			return;
		}
		void make_codewords(){
			int codeword = isIncr? 0: 1 << (maxLen - 1);// integer version
			int i = 0; // symbol index
			for(int l = 1; l <= maxLen; l++){
				for(int j = 0; j < nCodesOfLen[l]; j++){
					if(!isIncr && i != 0) codeword -= 1 << (maxLen - l);
					fprintf(stderr, "Huffman.make_codewords(): i=%2d, l=%2d, codeword=%4d=", i, l, codeword);
					fprintb(stderr, codeword, maxLen);
					fprintf(stderr, "(");
					fprintb(stderr, codeword >> (maxLen - l), l);
					fprintf(stderr, "), symbol=%02X(%d)\n", symbol[i], symbol[i]);
					this->codeword[i] = codeword;
					this->codelen[i] = l;
					i++;
					if(isIncr) codeword += 1 << (maxLen - l - 1);
				}
			}
			this->codeword[i] = isIncr? nCodewords: 0; // ending symbol used for hash table
			return;
		}
	private:
		int get_symbol_index(unsigned char symbol){
			int i;
			for(i = 0; i < nSymbol; i++) if(this->symbol[i] == symbol) break; else if(i == nSymbol - 1) EXIT("Huffman.set_codeword(): error. // i");
			return i;
		}
	public:
		int get_codelen(unsigned char symbol){
			return codelen[get_symbol_index(symbol)];
		}
		void set_codeword(unsigned char symbol, const char *codeword_str){
			int i = get_symbol_index(symbol);
			int codeword = 0;
			int codelen = strlen(codeword_str);
			for(int j = 0; j < codelen; j++){
				int bit = codeword_str[j] - '0';
				if(bit != 0 && bit != 1) EXIT("Huffman.set_codeword(): error. // bit");
				codeword = codeword*2 + bit;
			}
			codeword <<= maxLen - codelen;
			this->codeword[i] = codeword;
			fprintf(stderr, "Huffman.set_codeword():   i=%2d, l=%2d, codeword=%4d=", i, codelen, this->codeword[i]);
			fprintb(stderr, this->codeword[i], maxLen);
			fprintf(stderr, "(");
			fprintb(stderr, this->codeword[i] >> (maxLen - codelen), codelen);
			fprintf(stderr, "), symbol=%02X(%d)\n", this->symbol[i], this->symbol[i]);
			return;
		}
		void make_hash_table(){
			// produce huffman hash function
			int i = isIncr? 0: nSymbol; // hashed symbol index
			for(int codeword = 0; codeword < nCodewords; codeword++){
				if(codeword >= this->codeword[isIncr? i + 1: i - 1]) isIncr? i++: i--;
				this->hash[codeword] = symbol[isIncr? i: (i >= 0)? i: 0];
				if(codeword == this->codeword[i]){
					fprintf(stderr, "\nHuffman.make_hash_table(): i=%2d, symbol[%2d]=%3d, codeword[%d]=%d <= %d < codeword[%d]=%d", i, i, symbol[i], i, this->codeword[i], codeword, isIncr? i + 1: i - 1, isIncr? this->codeword[i + 1]: (i - 1 >= 0)? this->codeword[i - 1]: nCodewords);
				}
				else if(codeword == this->codeword[isIncr? i: (i >= 0)? i: 0] + 1){
					fprintf(stderr, "...");
				}
			}
			fprintf(stderr, "\n\n");
			return;
		}
		unsigned char decode(int codeword){
			fprintf(stderr, "Huffman.decode(): codeword=%d=", codeword);
			fprintb(stderr, codeword, maxLen);
			unsigned char symbol = hash[codeword];
			fprintf(stderr, ", symbol=%02X(%d)\n", symbol, symbol);
			return symbol;
		}
};
#endif

