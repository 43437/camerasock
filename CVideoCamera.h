#ifndef __VIDEO_CAMERA__
#define __VIDEO_CAMERA__
#include <iostream>
extern "C"{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavdevice/avdevice.h>
	#include <libswscale/swscale.h>
	#include <libavutil/imgutils.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
}

#define PIXEL_W	640
#define PIXEL_H 360
#define DATA_SPLIT_LEN 8
#define BEGIN_FLAG "@BEGIN@@"
#define END_FLAG "@END@@@@"

class CVideoCamera{
  
public:
	CVideoCamera();
	~CVideoCamera();
	int Encode();
	void Terminal(){m_bStop = true;};
	void Start();

private:
	int OpenInput();
	int OpenOutput();
	void Init();
	void Release();
	void Publish(AVPacket& stuOutPkt);
	bool OpenConnection();

private:
	const char *m_strInputVideoSize;
	bool m_bStop;

	const char *m_strInputFmt;
	const char *m_strInputName;
	const char *m_strOutputFmt;
	const char *m_strOutputName;

	AVCodecContext *m_pInCodecCtx;
	AVFormatContext *m_pInFmtCtx;

	AVCodecContext *m_pOutCodecCtx;
	int m_iSockConn;
	const char* m_arrRemoteAddr;
	const int m_iRemotePort;

	SwsContext *m_pSwsCtx;
	enum AVPixelFormat m_eDstPixFmt;

	enum{
		eSendBufSize = 1024000,
	};
	uint8_t m_arrSendBuf[eSendBufSize];
};

#endif
