
#ifndef _HUFFMAN_H
#define _HUFFMAN_H

#include <vector>
#include <algorithm>

//8bit each level
class HTABLE{
    public:
        std::vector<int*> rs;
        std::vector<char*> rl;
        void load(const char *filename);
        ~HTABLE();
};


#endif
