#include <stdio.h>
  
// OpenCV
#include <opencv2/opencv.hpp>
 
// FFmpeg
extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  #include "libswscale/swscale.h"
}
 
// --------------------------------------------------------------------------
// main(引数の数、引数リスト)
// メイン関数です
// 戻り値 正常終了:0 エラー:-1
// --------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // AR.Droneのアドレス
    char *drone_addr = "tcp://192.168.1.1:5555";
 
    // FFmpeg初期化
    av_register_all();
    avformat_network_init();
 
    // ビデオを開く
    AVFormatContext *pFormatCtx = NULL;
    if (avformat_open_input(&pFormatCtx, drone_addr, NULL, NULL) != 0) {
        printf("ERROR: avformat_open_input() failed. (%s, %d)\n", __FILE__, __LINE__);
        return -1;
    }
 
    // ストリーム情報の取得
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) return -1;
    av_dump_format(pFormatCtx, 0, drone_addr, 0);
 
    // コーデックの検索
    AVCodecContext *pCodecCtx = pFormatCtx->streams[0]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
 
    // コーデックを開く
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("ERROR: avcodec_open2() failed. (%s, %d)\n", __FILE__, __LINE__);
        return -1;
    }
 
    // ビデオ
    AVFrame *pFrame = avcodec_alloc_frame();
    AVFrame *pFrameBGR = avcodec_alloc_frame();
 
    // バッファの確保
    uchar *buffer = (uchar*)av_malloc(avpicture_get_size(PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height));
 
    // バッファと画像を関連付ける
    avpicture_fill((AVPicture*)pFrameBGR, buffer, PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
 
    // 変換コンテキスト
    SwsContext *pConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,  pCodecCtx->width, pCodecCtx->height, PIX_FMT_BGR24, SWS_SPLINE, NULL, NULL, NULL);
 
    // 画像用のメモリ確保
    IplImage *img = cvCreateImage(cvSize(pCodecCtx->width, pCodecCtx->height), IPL_DEPTH_8U, 3);
 
    // メインループ
    while (1) {
        AVPacket packet;
        int frameFinished;
 
        // フレーム読み込み
        while (av_read_frame(pFormatCtx, &packet) >= 0) {
            // デコード
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
 
            // デコード完了
            if (frameFinished) {
                sws_scale(pConvertCtx, (const uchar* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameBGR->data, pFrameBGR->linesize);
                memcpy(img->imageData, pFrameBGR->data[0], pCodecCtx->width * pCodecCtx->height * sizeof(uchar) * 3);
            }
 
            // メモリ解放
            av_free_packet(&packet);
        }
 
        // 表示
        cvShowImage("img", img);
        if (cvWaitKey(1) == 0x1b) break;
    }
 
    // 画像用のメモリ解放
    cvReleaseImage(&img);
 
    // FFmpeg終了
    sws_freeContext(pConvertCtx);
    av_free(buffer);
    av_free(pFrameBGR);
    av_free(pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
  
    return 0;
}
