#ifndef _MPEG1_H
#define _MPEG1_H

#include <stdio.h>
#include <string.h>
#include <vector>
#include "huffman.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

class PICTURE{
    public:
        u8* _y;
        u8* _cb;
        u8* _cr;
        int _sz;
        int W,H,i;
        PICTURE(int height,int width,int _i){
            i = _i;
            int sz = height*width;
            W = width;
            H = height;
            _y = new u8[sz];
            _cb = new u8[sz];
            _cr = new u8[sz];
            memset(_y,0,sz);
            memset(_cb,0,sz);
            memset(_cr,0,sz);
            _sz = sz;
        }
        ~PICTURE(){
            delete[] _y;
            delete[] _cb;
            delete[] _cr;
        }
        u8* _ptr(int m,int n,int b){
            u8 *p;
            switch(b){
                case 0:
                    p = _y+(m<<4)*W+(n<<4);
                    break;
                case 1:
                    p = _y+(m<<4)*W+(n<<4)+8;
                    break;
                case 2:
                    p = _y+((m<<4)+8)*W+(n<<4);
                    break;
                case 3:
                    p = _y+((m<<4)+8)*W+(n<<4)+8;
                    break;
                case 4:
                    p = _cb+(m<<3)*W+(n<<3);
                    break;
                default: //5
                    p = _cr+(m<<3)*W+(n<<3);
            }
            return p;
        }
        void getBlock(int x[9][9],int m,int n,int down_for,int right_for,int b,int v,int h){
            u8 *p = _ptr(m,n,b)+down_for*W+right_for;
            for( int i=0; i<v; i++ ){
                for( int j=0; j<h; j++ ){
                    x[i][j] = *p++;
                }
                p += W-h;
            }
        }
        void setBlock(int x[8][8],int m,int n,int b){
            u8 *p = _ptr(m,n,b);
            for( int i=0; i<8; i++ ){
                for( int j=0; j<8; j++ ){
                    *p++ = x[i][j];
                }
                p += W-8;
            }
        }
        int clip(int x){
            if(x<0) return 0;
            if(x>255) return 255;
            return x;
        }
        void clear(){
            memset(_y,0,_sz);
            memset(_cb,0,_sz);
            memset(_cr,0,_sz);
        }
        void saveBMP(int id){
            char filename[32];
            sprintf(filename,"BMP\\BMP_%06d.bmp",id);
            int X=720;
            int Y=400;
            int IMG[720*400];
            //high quality resample, but slower
            /*u8* tmpcb = new u8[(X+1)*(Y+1)];
            for( int i=1; i<Y; i+=2 ){
                for( int j=1; j<X; j+=2 ){
                    tmpcb[i*(X+1)+j]=_cb[(i/2)*W+(j/2)];
                    if(j!=X-1) tmpcb[i*(X+1)+j+1]=(_cb[(i/2)*W+(j/2)]+_cb[(i/2)*W+(j/2)+1])/2;
                    if(i!=Y-1) tmpcb[(i+1)*(X+1)+j]=(_cb[(i/2)*W+(j/2)]+_cb[(i/2+1)*W+(j/2)])/2;
                    if(j!=X-1 && i!=Y-1) tmpcb[(i+1)*(X+1)+j+1]=
                        (_cb[(i/2)*W+(j/2)]+_cb[(i/2+1)*W+(j/2)]+_cb[(i/2)*W+(j/2)+1]+_cb[(i/2+1)*W+(j/2)+1])/4;
                }
                tmpcb[i*(X+1)]=_cb[(i/2)*W];
                tmpcb[i*(X+1)+X]=_cb[(i/2)*W+((X-1)/2)];
                if(i!=Y-1){
                    tmpcb[(i+1)*(X+1)]=(_cb[(i/2)*W]+_cb[(i/2+1)*W])/2;
                    tmpcb[(i+1)*(X+1)+X]=(_cb[(i/2)*W+((X-1)/2)]+_cb[(i/2+1)*W+((X-1)/2)])/2;
                }
            }
            u8* tmpcr = new u8[(X+1)*(Y+1)];
            for( int i=1; i<Y; i+=2 ){
                for( int j=1; j<X; j+=2 ){
                    tmpcr[i*(X+1)+j]=_cr[(i/2)*W+(j/2)];
                    if(j!=X-1) tmpcr[i*(X+1)+j+1]=(_cr[(i/2)*W+(j/2)]+_cr[(i/2)*W+(j/2)+1])/2;
                    if(i!=Y-1) tmpcr[(i+1)*(X+1)+j]=(_cr[(i/2)*W+(j/2)]+_cr[(i/2+1)*W+(j/2)])/2;
                    if(j!=X-1 && i!=Y-1) tmpcr[(i+1)*(X+1)+j+1]=
                        (_cr[(i/2)*W+(j/2)]+_cr[(i/2+1)*W+(j/2)]+_cr[(i/2)*W+(j/2)+1]+_cr[(i/2+1)*W+(j/2)+1])/4;
                }
                tmpcr[i*(X+1)]=_cr[(i/2)*W];
                tmpcr[i*(X+1)+X]=_cr[(i/2)*W+((X-1)/2)];
                if(i!=Y-1){
                    tmpcr[(i+1)*(X+1)]=(_cr[(i/2)*W]+_cr[(i/2+1)*W])/2;
                    tmpcr[(i+1)*(X+1)+X]=(_cr[(i/2)*W+((X-1)/2)]+_cr[(i/2+1)*W+((X-1)/2)])/2;
                }
            }*/
            for( int i=0; i<Y; i++ ){
                for( int j=0; j<X; j++ ){
                    double Y = _y[i*W+j]-16;
                    double Cb = _cb[(i>>1)*W+(j>>1)]-128;
                    double Cr = _cr[(i>>1)*W+(j>>1)]-128;
                    //double Cb = (tmpcb[i*(X+1)+j]+tmpcb[(i+1)*(X+1)+j]+tmpcb[i*(X+1)+j+1]+tmpcb[(i+1)*(X+1)+j+1])/4-128;
                    //double Cr = (tmpcr[i*(X+1)+j]+tmpcr[(i+1)*(X+1)+j]+tmpcr[i*(X+1)+j+1]+tmpcr[(i+1)*(X+1)+j+1])/4-128;
                    int R = clip(255/219.0*Y+255/112.0*0.701*Cr);
                    int G = clip(255/219.0*Y-255/112.0*0.886*0.114/0.587*Cb-255/112.0*0.701*0.299/0.587*Cr);
                    int B = clip(255/219.0*Y+255/112.0*0.886*Cb);
                    IMG[i*W+j] = ((R<<16)|(G<<8)|B);
                }
            }
            //delete[] tmpcb;
            //delete[] tmpcr;
            FILE *file = fopen(filename,"wb");
            int header[3] = { ((X*24+31)/32)*4*Y+54, 0, 54 };
            int DIB[10]={
                40,
                X,
                Y,
                0x180001,
                0,
                ((X*24+31)/32)*4*Y,
                3780,
                3780,
                0,
                0
            };
            int pad = 0;
            int padlen = ((X*24+31)/32)*4-X*3;
            char *output = new char[X*3+padlen];
            fwrite("BM",1,2,file);
            fwrite(header,4,3,file);
            fwrite(DIB,4,10,file);
            for( int y=Y-1; y>=0; y-- ){
                for( int x=0; x<X; x++ ){
                    output[x*3]=(IMG[y*W+x])&0xff;
                    output[x*3+1]=(IMG[y*W+x]>>8)&0xff;
                    output[x*3+2]=(IMG[y*W+x]>>16)&0xff;
                }
                fwrite(output,1,X*3+padlen,file);
            }
            fclose(file);
            delete output;
        }
};

class GOP{
    public:
        int pic;
        int tick;
        int pos;
        int header;
        GOP(){}
        GOP(int _pic,int _tick,int _pos,int _header){
            pic = _pic;
            tick = _tick;
            pos = _pos;
            header = _header;
        }
};

class MPEG1{
    private:
        FILE *finput;
        u32 *inputBuffer;
        int inputBufPtr;
        void checkBuffer(int num);
        void skipBits(int num);
        u32 readBits(int num);
        u32 readBits32();
        u32 nextBits(int num);
        u32 nextBits32();
        int next_start_code();
        void marker_bit();
        void skip_extra_data();
        int vlc_decode(const HTABLE *ht);

        int vlc_macro_block_address();
        HTABLE ht1;

        int vlc_macro_block_type();
        HTABLE ht2a;
        HTABLE ht2b;
        HTABLE ht2c;

        int vlc_coded_block_pattern();
        HTABLE ht3;

        int vlc_motion_vector_code();
        HTABLE ht4;

        int vlc_dct_dc_size_luminance();
        HTABLE ht5a;

        int vlc_dct_dc_size_chrominance();
        HTABLE ht5b;

        int vlc_coeff(int first);
        HTABLE ht5c;
        HTABLE ht5d;

    private:
        int video_sequence(double pos);

        int sequence_header();
        int mb_width;
        int mb_height;
        int bit_rate;
        int vbv_buffer_size;
        int constrained_parameter_flag;

        int fast_forward();
        std::vector<GOP> gop_filepos;
        int last_header;
        int picture_counter;
        int total_picture;

        int group_of_pictures();
        int time_code;
        int drop_frame_flag;
        int time_code_hours;
        int time_code_minutes;
        int time_code_seconds;
        int time_code_pictures;
        int closed_gop;
        int broken_link;

        int picture();
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

        int slice();
        int slice_start_code;
        int slice_vertical_position;
        int quantizer_scale;

        int macro_block();
        int macroblock_address_increment;
        int macroblock_address;
        int mb_row;
        int mb_column;
        int macroblock_type;
        int macroblock_quant;
        int macroblock_motion_forward;
        int macroblock_motion_backward;
        int macroblock_pattern;
        int macroblock_intra;
        int past_intra_address;
        int motion_horizontal_forward_code;
        int motion_vertical_forward_code;
        int motion_horizontal_backward_code;
        int motion_vertical_backward_code;
        int motion_horizontal_forward_r;
        int motion_vertical_forward_r;
        int motion_horizontal_backward_r;
        int motion_vertical_backward_r;
        int coded_block_pattern;
        int pattern_code[6];

        int block(int i);
        int dct_dc_size_luminance;
        int dct_dc_size_chrominance;
        int dct_dc_differential;
        int dct_coeff_first;
        int dct_coeff_next;

        int dct_recon[8][8];
        int intra_quant[8][8];
        int non_intra_quant[8][8];
        int dct_zz[64];
        int dct_dc_y_past;
        int dct_dc_cb_past;
        int dct_dc_cr_past;
        int recon_down_for_prev;
        int recon_right_for_prev;
        int recon_right_for;
        int recon_down_for;
        int recon_down_back_prev;
        int recon_right_back_prev;
        int recon_right_back;
        int recon_down_back;
        int pel[9][9];
        int pel_past[9][9];

        int motion_vector_P();
        int motion_vector_B();
        int intra_coded_block_decode(int i);
        int predictive_block_P_decode(int i);
        int macroblock_skipped_P();
        int predictive_block_B_decode(int i);
        int macroblock_skipped_B();

        int gop_counter;
        int pic_queue_ptr;
        int pic_queue_counter;
        int pic_queue_end;
        PICTURE* picBuf[32];
        PICTURE* picL;
        PICTURE* picC;
        PICTURE* picR;

        int closeFileFlag;
        int decodingFlag;
    public:
        int horizontal_size;
        int vertical_size;
        double pel_aspect_ratio;
        double picture_rate;
        
        PICTURE* nextPicture();
        MPEG1();
        int loadInfo(const char *filename);
        int open(const char *filename,double pos=0);
        int close();
        double getPos();
};



#endif
