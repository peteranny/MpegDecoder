#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <time.h>
#include <string.h>
#include <process.h>
#include "resource.h"
#include "mpeg1.h"
using namespace std;

const  char* szAppName = "MyMPEGViewerWndClass";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND hwnd;
MPEG1 mpeg;
void show_frame();
void copyPictureToScreen();
void openFile();
void background_decode(void*);
void scale(double r);

char szFile[1024];      
int fileOpenFlag;
double filePos;

char windowCaption[100];
double averageRate;
double recentRate[10];
int recentRatePtr;
int lastTime;
int timeTotal;
int frameTotal;

HDC hdc;
HDC memdc;
HBITMAP memBmp;
HBITMAP bufBmp;
int *pBuf;
int picBufSize;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
        LPSTR lpCmdLine, int nCmdShow){
    WNDCLASSEX wc;
    MSG Msg;

    FreeConsole();

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(NULL,IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
    wc.lpszClassName = szAppName;
    wc.hIconSm = LoadIcon(NULL,IDI_APPLICATION);

    RegisterClassEx(&wc);

    hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,szAppName,
            "MPEG viewer",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
            NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    hdc = GetDC(hwnd);
    memdc=CreateCompatibleDC(hdc);
    copyPictureToScreen();

    fileOpenFlag = 0;
    while(1){
        if(PeekMessage(&Msg,NULL,0,0,0)){
            if(GetMessage(&Msg,0,0,0)>0){
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }else{
                break;
            }
        }
        show_frame();
    }
    return Msg.wParam;
}

void mouseDown(int x,int y){
    if(!fileOpenFlag) return;
    RECT rc;
    GetClientRect(hwnd,&rc);
    if(y<rc.bottom-35) return;
    int width=rc.right;
    double pos=(double)(x-10)/(width-20);
    if(pos<0) pos=0;
    if(pos>1) pos=1;
    mpeg.close();

    filePos = pos;
    _beginthread(background_decode,0,NULL);

    fileOpenFlag = 1;
    timeTotal = 0;
    frameTotal = 0;
    averageRate = 0;
    recentRatePtr = 0;
    lastTime = 0;
    for( int i=0; i<10; i++ ) recentRate[i]=0;
}

LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_SIZE:
            copyPictureToScreen();
            break;
        case WM_COMMAND:
            switch(LOWORD(wParam)){
                case ID_FILE_OPEN:
                    openFile();
                    break;
                case ID_FILE_EXIT:
                    DestroyWindow(hwnd);
                    return 0;
                case ID_VIEW_100:
                    scale(1.0);
                    break;
                case ID_VIEW_50:
                    scale(0.5);
                    break;
                case ID_VIEW_200:
                    scale(2.0);
                    break;
            }
            break;
        case WM_LBUTTONDOWN:
            mouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void background_decode(void*){
    mpeg.open(szFile,filePos);
}

void loadFile(){
    filePos = 0;
    mpeg.loadInfo(szFile);
    fileOpenFlag = 1;
    scale(1.0);
    _beginthread(background_decode,0,NULL);

    memBmp=CreateCompatibleBitmap(hdc,mpeg.horizontal_size,mpeg.vertical_size);
    picBufSize = mpeg.horizontal_size*mpeg.vertical_size;
    if(pBuf) delete pBuf;
    pBuf = new int[picBufSize];
    HBITMAP pOldBmp=(HBITMAP)SelectObject(memdc,memBmp);

    timeTotal = 0;
    frameTotal = 0;
    averageRate = 0;
    recentRatePtr = 0;
    lastTime = 0;
    for( int i=0; i<10; i++ ) recentRate[i]=0;
}

inline int clip(int x){
    if(x<0) return 0;
    if(x>255) return 255;
    return x;
}

void show_frame(){
    int curTime;
    if(fileOpenFlag){
        while(1){
            curTime = timeGetTime();
            if(frameTotal<=(timeTotal+curTime-lastTime)*mpeg.picture_rate/1000.0) break;
            Sleep(1);
        }
        timeTotal += min(curTime-lastTime,50);
        frameTotal++;
        double rate = 1000.0/(curTime-lastTime);
        lastTime = curTime;
        averageRate = (averageRate*10+rate-recentRate[recentRatePtr])/10;
        recentRate[recentRatePtr] = rate;
        recentRatePtr = (recentRatePtr+1)%10;

        sprintf(windowCaption,"FPS = %.2f, aspect ratio = %.4f\n",averageRate,mpeg.pel_aspect_ratio);
        SetWindowText(hwnd,windowCaption);

        PICTURE* pic = mpeg.nextPicture();
        if(pic!=NULL){
            for( int i=0; i<mpeg.vertical_size; i++ ){
                for( int j=0; j<mpeg.horizontal_size; j++ ){
                    double Y = pic->_y[i*mpeg.horizontal_size+j]-16;
                    double Cb = pic->_cb[(i>>1)*mpeg.horizontal_size+(j>>1)]-128;
                    double Cr = pic->_cr[(i>>1)*mpeg.horizontal_size+(j>>1)]-128;
                    int R = clip(255/219.0*Y+255/112.0*0.701*Cr);
                    int G = clip(255/219.0*Y-255/112.0*0.886*0.114/0.587*Cb-255/112.0*0.701*0.299/0.587*Cr);
                    int B = clip(255/219.0*Y+255/112.0*0.886*Cb);
                    pBuf[i*mpeg.horizontal_size+j] = ((R<<16)|(G<<8)|B);
                }
            }
        }else{
            for( int i=0; i<mpeg.vertical_size; i++ ){
                for( int j=0; j<mpeg.horizontal_size; j++ ){
                    pBuf[i*mpeg.horizontal_size+j] = 0;
                }
            }
        }
        SetBitmapBits(memBmp,picBufSize*4,pBuf);
    }else{
        while(1){
            curTime = timeGetTime();
            if(curTime-lastTime>10) break;
            Sleep(1);
        }
        lastTime = curTime;
    }
    copyPictureToScreen();
}

void copyPictureToScreen(){
    RECT rc;
    GetClientRect(hwnd,&rc);
    int W=rc.right;
    int H=rc.bottom;
    static int barPos = 10;
    double newPos = mpeg.getPos();
    if(newPos>=0 && newPos<=1){
        barPos = (int)(newPos*(rc.right-20))+10;
    }

    HDC bufdc=CreateCompatibleDC(hdc);
    HBITMAP bufBmp=CreateCompatibleBitmap(hdc,W,H);
    SelectObject(bufdc,bufBmp);

    if(fileOpenFlag){
        double rx=(double)W/mpeg.horizontal_size;
        double ry=(double)H/(mpeg.vertical_size*mpeg.pel_aspect_ratio);
        double r=rx<ry?rx:ry;
        int nX = (int)(mpeg.horizontal_size*r);
        int nY = (int)(mpeg.vertical_size*r*mpeg.pel_aspect_ratio);

        SetStretchBltMode(bufdc,HALFTONE);
        StretchBlt(bufdc,(W-nX)/2,(H-nY)/2,nX,nY,memdc,0,0,mpeg.horizontal_size,mpeg.vertical_size,SRCCOPY);
    }
    POINT curpos;
    RECT rcn;
    GetWindowRect(hwnd,&rcn);
    GetCursorPos(&curpos);
    if(curpos.x>rcn.left+10 && curpos.x<rcn.right-10 && curpos.y>rcn.bottom-50 && curpos.y<rcn.bottom-10){
        RoundRect(bufdc,10,rc.bottom-20,rc.right-10,rc.bottom-15,4,4);
        RoundRect(bufdc,barPos-5,rc.bottom-30,barPos+5,rc.bottom-5,5,5);
    }
    BitBlt(hdc,0,0,W,H,bufdc,0,0,SRCCOPY);

    DeleteObject(bufBmp);
    DeleteDC(bufdc);
}

void scale(double r){
    RECT rc,rcn;
    GetWindowRect(hwnd,&rcn);
    rc.left = 0;
    rc.top = 0;
    if(fileOpenFlag){
        rc.right = (int)(mpeg.horizontal_size*r);
        rc.bottom = (int)(mpeg.vertical_size*r);
    }else{
        rc.right = 100;
        rc.bottom = 100;
    }
    AdjustWindowRectEx(&rc,WS_OVERLAPPEDWINDOW,true,WS_EX_OVERLAPPEDWINDOW);
    //printf("%d %d %d %d\n",rc.left,rc.right,rc.top,rc.bottom);
    SetWindowPos(hwnd,HWND_TOP,rcn.left,rcn.top,rc.right-rc.left,rc.bottom-rc.top,0);
}

void openFile(){
    OPENFILENAME ofn;       
    HANDLE hf;              

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0MPEG-1 Video Only (*.m1v)\0*.m1v\0";
    ofn.nFilterIndex = 2;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)==TRUE){
        if(!strcmpi(szFile+strlen(szFile)-4,".m1v")){
            mpeg.close();
            loadFile();
            Sleep(10);
        }else{
            MessageBox(hwnd,"File format not support","Error",MB_ICONERROR);
        }
    }
}

