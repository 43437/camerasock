/*
 * CVideoReceiver.cpp
 *
 *  Created on: Feb 9, 2020
 *      Author: acmax
 */

#include "CVideoReceiver.h"
#include <thread>
#include "CLogUtil.h"

#include "CVideoCamera.h"

void Run();

CVideoReceiver::CVideoReceiver():
		m_iSDLBufSize(1024000),
		m_bStop(false),
		m_arrLocalAddr("127.0.0.1"),
		m_iLocalPort(1234)
{
	// TODO Auto-generated constructor stub

	m_pSDLFrameBuff = (uint8_t*) malloc (m_iSDLBufSize);
	if ( NULL == m_pSDLFrameBuff ){
		LogUtil.Log(CLogUtil::eWTF, "malloc m_pSDLFrameBuff error. ");
	}
}

CVideoReceiver::~CVideoReceiver() {
	// TODO Auto-generated destructor stub
	free(m_pSDLFrameBuff);
}

void CVideoReceiver::Start(){

	std::thread t(&CVideoReceiver::Receive, this);
	std::thread t1(&CVideoReceiver::Decode, this, m_pSDLFrameBuff, m_iSDLBufSize);
	t.detach();
	t1.detach();
}

uint8_t *CVideoReceiver::GetBuffer(){

	return m_pSDLFrameBuff;
}

int CVideoReceiver::InitContext(AVCodecContext *pSDLCodecCtx){

	pSDLCodecCtx->codec_id = AV_CODEC_ID_H264;
	pSDLCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pSDLCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pSDLCodecCtx->width = PIXEL_W;
	pSDLCodecCtx->height = PIXEL_H;
	pSDLCodecCtx->time_base.num =1;
	pSDLCodecCtx->time_base.den = 2;
	pSDLCodecCtx->bit_rate = 100000;
	pSDLCodecCtx->gop_size=5;
	pSDLCodecCtx->qmin = 10;
	pSDLCodecCtx->qmax = 51;
//	m_pSDLCodecCtx->delay = 0;
//	m_pSDLCodecCtx->flags |= CODEC_FLAG_LOW_DELAY;
	AVCodec *pIncodec = avcodec_find_decoder(pSDLCodecCtx->codec_id);
	if (!pIncodec){
		LogUtil.Log(CLogUtil::eWTF, "InitContext find avcodec failed. ");
		return -1;
	}

	if(avcodec_open2(pSDLCodecCtx, pIncodec, NULL) < 0){
		LogUtil.Log(CLogUtil::eWTF, "InitContext avcodec open m_pSDLCodecCtx failed.");
		return -1;
	}

	return 0;
}

//接收线程
void CVideoReceiver::Receive(){

	//网络
	int iServSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//将套接字和IP、端口绑定
	struct sockaddr_in stuServAddr;
	memset(&stuServAddr, 0, sizeof(stuServAddr));  //每个字节都用0填充
	stuServAddr.sin_family = AF_INET;  //使用IPv4地址
	stuServAddr.sin_addr.s_addr = inet_addr(m_arrLocalAddr);  //具体的IP地址
	stuServAddr.sin_port = htons(m_iLocalPort);  //端口
	bind(iServSock, (struct sockaddr*)&stuServAddr, sizeof(stuServAddr));

	//进入监听状态，等待用户发起请求
	listen(iServSock, 20);

	//接收客户端请求
	struct sockaddr_in stuClntAddr;
	socklen_t lenAddrSize = sizeof(stuClntAddr);
	int iClntSock = accept(iServSock, (struct sockaddr*)&stuClntAddr, &lenAddrSize);

	int iLen = 0;
	const int iRcvBufSize = 1024000;
	uint8_t arrRcvBuf[1024000] = {0};

	while ( (iLen = recv(iClntSock, arrRcvBuf, iRcvBufSize, 0) ) > 0 ){

		LogUtil.Log(CLogUtil::eDebug, "recv len:[%d]", iLen);
		CDataPool::GetInstance().AppendBufferData(arrRcvBuf, iLen);
	}
}

//解码线程
void CVideoReceiver::Decode(uint8_t* pBuff, const int& iLen){

	//编解码
	AVCodecContext *pSDLCodecCtx = avcodec_alloc_context3(NULL);
	if ( InitContext(pSDLCodecCtx) < 0 ){
		LogUtil.Log(CLogUtil::eWTF, "run InitContext failed.");
		return;
	}
	AVPacket *pPktRcv = NULL;
	AVFrame *pSDLFrame = av_frame_alloc();
	while(! m_bStop ){
		if (CDataPool::GetInstance().TakeNextPacketData(pPktRcv)){

			int iYUVDataLen = 0;
			int iRet = avcodec_send_packet(pSDLCodecCtx, pPktRcv);
			bool bRetry = false;
			LogUtil.Log(CLogUtil::eDebug, "decode code:[%d] =? [%d]", iRet, true);
			switch(iRet)
			{
			case AVERROR(EAGAIN):
				{
					bRetry = true;
				}
			case 0:
				{
					int iRecvRet = 0;
					while ( 0 == ( iRecvRet = avcodec_receive_frame(pSDLCodecCtx, pSDLFrame) ))
					{
						LogUtil.Log(CLogUtil::eDebug, "refresh sdl.");
						//SDL刷新画面m_pSDLFrameBuff大小m_iSDLBufSize
						if ( ( iYUVDataLen = av_image_copy_to_buffer(pBuff, iLen,
												pSDLFrame->data, pSDLFrame->linesize,
												(AVPixelFormat)pSDLFrame->format,
												pSDLFrame->width, pSDLFrame->height, 1) ) < 0 ){
							LogUtil.Log(CLogUtil::eWTF, "av_image_copy_to_buffer failed.");
						}
						else{
							LogUtil.Log(CLogUtil::eDebug, "av_image_copy_to_buffer len:[%d]", iYUVDataLen);
						}
						SDL_Event event;
						event.type = REFRESH_EVENT;
						SDL_PushEvent(&event);
					}

					if (bRetry)
					{
						avcodec_send_packet(pSDLCodecCtx, pPktRcv);
					}
				}
				break;
//				AVERROR_EOF:
//				AVERROR(ENOMEM):
//					AVERROR(EINVAL):
			default:
				break;
			}

			CDataPool::GetInstance().DisardNextPacketData();
		}
		else{
			LogUtil.Log(CLogUtil::eDebug, "TakeNextPacketData NULL.");
		}
	}
}
