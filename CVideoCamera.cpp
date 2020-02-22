#include <cstring>
#include <cstdlib>
#include "CVideoCamera.h"
#include "CLogUtil.h"
#include <thread>

CVideoCamera::CVideoCamera()
					:m_strInputVideoSize("640x360"),
					 m_bStop(false),
					 m_strInputFmt("video4linux2"),
					 m_strInputName("/dev/video0"),
					 m_strOutputFmt("h264"),
					 m_strOutputName("out.h264"),
					 m_pInCodecCtx(NULL),
					 m_pInFmtCtx(NULL),
					 m_pOutCodecCtx(NULL),
					 m_iSockConn(-1),
					 m_arrRemoteAddr("127.0.0.1"),
					 m_iRemotePort(1234),
					 m_pSwsCtx(NULL),
					 m_eDstPixFmt(AV_PIX_FMT_YUV420P)
{
	Init();
}

CVideoCamera::~CVideoCamera()
{
	Release();
}

void CVideoCamera::Release()
{
	if ( NULL != m_pOutCodecCtx )
	{
		avcodec_free_context(&m_pOutCodecCtx);
		m_pOutCodecCtx = NULL;
	}

	if ( NULL != m_pInCodecCtx )
	{
		avcodec_free_context(&m_pInCodecCtx);
		m_pInCodecCtx = NULL;
	}
}

void CVideoCamera::Init()
{
	//   av_log_set_level(AV_LOG_DEBUG);
	if (NULL == m_pOutCodecCtx){
		m_pOutCodecCtx = avcodec_alloc_context3(NULL);
	}

	if (NULL == m_pInCodecCtx){
		m_pInCodecCtx = avcodec_alloc_context3(NULL);
	}

	OpenInput();
	OpenOutput();
}

//打开输入流
int CVideoCamera::OpenInput()
{
	//1. 获取视频流格式
	AVInputFormat *pInFmt = av_find_input_format(m_strInputFmt);
	if (pInFmt == NULL){
		LogUtil.Log(CLogUtil::eWTF, "can not find input format. return");
		return -1;
	}

	//2. 打开视频流
	AVDictionary *option = NULL;
	av_dict_set(&option, "video_size", m_strInputVideoSize, 0);
	if (avformat_open_input(&m_pInFmtCtx, m_strInputName, pInFmt, &option)<0){
		LogUtil.Log(CLogUtil::eWTF, "can not open input file. return");
		return -1;
	}
	//***********************

	//3. 找到视频流索引
	int videoindex = -1;
	for (unsigned int i=0; i < m_pInFmtCtx->nb_streams; i++){
		if ( m_pInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ){
			videoindex = i;
			break;
		}
	}
	if(videoindex == -1){
		LogUtil.Log(CLogUtil::eWTF, "find video stream failed, now return.");
		return -1;
	}
	//***********************

	//4. 获取输入编解码Context
	if (avcodec_parameters_to_context(m_pInCodecCtx, m_pInFmtCtx->streams[videoindex]->codecpar) < 0){
		LogUtil.Log(CLogUtil::eWTF, "avcodec_parameters_to_context failed. now return.");
		return -1;
	}
//	m_pInCodecCtx->flags |= CODEC_FLAG_LOW_DELAY;
	LogUtil.Log(CLogUtil::eInfo, "picture width:%d height:%d format:%d ", m_pInCodecCtx->width, m_pInCodecCtx->height, m_pInCodecCtx->pix_fmt);
	//***********************

	//5. 获取视频格式转换Context
	m_pSwsCtx = sws_getContext( m_pInCodecCtx->width, m_pInCodecCtx->height,
								m_pInCodecCtx->pix_fmt,
								m_pInCodecCtx->width, m_pInCodecCtx->height,
								m_eDstPixFmt,
								SWS_BILINEAR, NULL, NULL, NULL);
	//***********************

	return 0;
}

//打开输出流
int CVideoCamera::OpenOutput()
{
	//1. 获取保存视频流格式
	AVOutputFormat *pOutFmt = NULL;
	pOutFmt = av_guess_format(m_strOutputFmt,NULL, NULL);
	if (!pOutFmt)
	{
		LogUtil.Log(CLogUtil::eWTF, "av guess format failed, now return. ");
		return -1;
	}

	//3. 获取输出编解码Context
	m_pOutCodecCtx->codec_id = pOutFmt->video_codec;
	m_pOutCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	m_pOutCodecCtx->pix_fmt = m_eDstPixFmt;
	m_pOutCodecCtx->width = m_pInCodecCtx->width;
	m_pOutCodecCtx->height = m_pInCodecCtx->height;
	m_pOutCodecCtx->time_base.num =1;
	m_pOutCodecCtx->time_base.den = 2;
	m_pOutCodecCtx->bit_rate = 100000;
	m_pOutCodecCtx->gop_size=5;
	m_pOutCodecCtx->qmin = 10;
	m_pOutCodecCtx->qmax = 51;
//	m_pOutCodecCtx->delay = 0;
//	m_pOutCodecCtx->flags |= CODEC_FLAG_LOW_DELAY;

	AVCodec *pOutcodec = avcodec_find_encoder(m_pOutCodecCtx->codec_id);
	if (!pOutcodec){
		LogUtil.Log(CLogUtil::eWTF, "find avcodec failed. return ");
		return -1;
	}

	// Set Option
	AVDictionary *param = 0;
	//H.264
	if(m_pOutCodecCtx->codec_id == AV_CODEC_ID_H264){
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		LogUtil.Log(CLogUtil::eInfo, "out codec is h264. ");
		//av_dict_set(¶m, "profile", "main", 0);
	}

	if(avcodec_open2(m_pOutCodecCtx, pOutcodec, NULL) < 0){
		LogUtil.Log(CLogUtil::eWTF, "output avcodec open failed. return");
		return -1;
	}

	return 0;
}

int CVideoCamera::Encode()
{
	int iRet = -1;
	uint8_t *parrSrcData[4];
	uint8_t *parrDstData[4];
	int iarrSrcLinesize[4];
	int iarrDstLinesize[4];

	av_image_alloc(parrSrcData, iarrSrcLinesize, m_pInCodecCtx->width, m_pInCodecCtx->height, m_pInCodecCtx->pix_fmt, 16);
	int dst_bufsize = av_image_alloc(parrDstData, iarrDstLinesize, m_pInCodecCtx->width, m_pInCodecCtx->height, m_eDstPixFmt, 16);

	int outBufSize = av_image_get_buffer_size(m_pOutCodecCtx->pix_fmt, m_pOutCodecCtx->width, m_pOutCodecCtx->height, 16);
	LogUtil.Log(CLogUtil::eInfo, "out buf size: %d", outBufSize);

	AVFrame *pOutFrame = av_frame_alloc();
	if (!pOutFrame){
		LogUtil.Log(CLogUtil::eWTF, "out frame alloc failed, now return. ");
		return -1;
	}
	uint8_t *pFrameBuff = (uint8_t *)av_malloc(outBufSize);
	if (!pFrameBuff){
		LogUtil.Log(CLogUtil::eWTF, "output frame buf alloc failed.");
		return -1;
	}

	//关联AVFrame *pOutFrame和 pFrameBuff
	av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, pFrameBuff, m_pOutCodecCtx->pix_fmt, m_pOutCodecCtx->width, m_pOutCodecCtx->height, 16);
	pOutFrame->width=m_pInCodecCtx->width;
	pOutFrame->height=m_pInCodecCtx->height;
	pOutFrame->format=m_pOutCodecCtx->pix_fmt;
	LogUtil.Log(CLogUtil::eInfo, "m_pInCodecCtx->pix_fmt[%d]", m_pInCodecCtx->pix_fmt);

	AVOutputFormat *pInFmt = NULL;
	pInFmt = av_guess_format(m_strOutputFmt,NULL, NULL);
	if (!pInFmt){
		LogUtil.Log(CLogUtil::eWTF, "output av guess format failed, now return.");
		return -1;
	}

	//开始
	AVPacket stuInPkt;
	AVPacket stuOutPkt;
	memset(&stuInPkt, 0, sizeof(AVPacket));
	av_init_packet(&stuInPkt);
	memset(&stuOutPkt, 0, sizeof(AVPacket));
	av_init_packet(&stuOutPkt);

	int i = 0;
	LogUtil.Log(CLogUtil::eInfo, "video push begin!");
	while (!m_bStop){
		i++;
		//从视频流中获取
		av_read_frame(m_pInFmtCtx, &stuInPkt);

		//视频格式变换
		memcpy(parrSrcData[0], stuInPkt.data, stuInPkt.size);
		sws_scale(m_pSwsCtx, parrSrcData, iarrSrcLinesize, 0, m_pInCodecCtx->height, parrDstData, iarrDstLinesize);
		memcpy(pFrameBuff, parrDstData[0], dst_bufsize);

		//编码
		pOutFrame->pts=i*(m_pOutCodecCtx->time_base.den)/((m_pOutCodecCtx->time_base.num)*2);
		if ( 0 != avcodec_send_frame(m_pOutCodecCtx, pOutFrame) ){
			LogUtil.Log(CLogUtil::eWTF, "avcodec_send_frame failed. return ");
			return -1;
		}

		//获取编码结果保存到输出流
		iRet = 0;
		while( 0 == iRet){
			iRet = avcodec_receive_packet(m_pOutCodecCtx, &stuOutPkt);
			switch(iRet)
			{
			case 0:
				stuOutPkt.stream_index = 0;
				Publish(stuOutPkt);
				//发送出去
				break;
			case AVERROR(EAGAIN):
				break;
			default:
				LogUtil.Log(CLogUtil::eWTF, "avcodec_receive_packet error. terminal ");
				Terminal();
				break;
			}
		}
	}

	avcodec_close(m_pInCodecCtx);
	avcodec_close(m_pOutCodecCtx);

	avformat_close_input(&m_pInFmtCtx);
	avformat_free_context(m_pInFmtCtx);

	av_packet_unref(&stuInPkt);
	av_packet_unref(&stuOutPkt);

	av_freep(parrDstData);
	av_freep(parrSrcData);
	sws_freeContext(m_pSwsCtx);

	Release();

	return 0;
}

void CVideoCamera::Publish(AVPacket& stuOutPkt)
{
	const int iLen = DATA_SPLIT_LEN + sizeof(stuOutPkt.pts) + sizeof(stuOutPkt.dts) + sizeof(stuOutPkt.stream_index) + sizeof(stuOutPkt.flags) + sizeof(stuOutPkt.size) + stuOutPkt.size + DATA_SPLIT_LEN;
	int iIndex = 0;
	memcpy(&m_arrSendBuf[iIndex], BEGIN_FLAG, DATA_SPLIT_LEN);
	iIndex += DATA_SPLIT_LEN;
	memcpy(&m_arrSendBuf[iIndex], &stuOutPkt.pts, sizeof(stuOutPkt.pts));	//pts
	iIndex += sizeof(stuOutPkt.pts);
	memcpy(&m_arrSendBuf[iIndex], &stuOutPkt.dts, sizeof(stuOutPkt.dts));	//dts
	iIndex += sizeof(stuOutPkt.dts);
	memcpy(&m_arrSendBuf[iIndex], &stuOutPkt.stream_index, sizeof(stuOutPkt.stream_index));	//stream_index
	iIndex += sizeof(stuOutPkt.stream_index);
	memcpy(&m_arrSendBuf[iIndex], &stuOutPkt.flags, sizeof(stuOutPkt.flags));	//flags
	iIndex += sizeof(stuOutPkt.flags);
	memcpy(&m_arrSendBuf[iIndex], &stuOutPkt.size, sizeof(stuOutPkt.size));	//size
	iIndex += sizeof(stuOutPkt.size);

	memcpy(&m_arrSendBuf[iIndex], stuOutPkt.data, stuOutPkt.size);	//data
	iIndex += stuOutPkt.size;
	memcpy(&m_arrSendBuf[iIndex], END_FLAG, DATA_SPLIT_LEN);

	int iSendLen = 0;
	if ( ( iSendLen = send(m_iSockConn, m_arrSendBuf, iLen, 0) ) < 0){
		LogUtil.Log(CLogUtil::eWTF, "CVideoCamera::publish failed. ");
	}
	else{
		LogUtil.Log(CLogUtil::eDebug, "data iLen: [%d] send len: [%d]", iLen, iSendLen);
	}
}

bool CVideoCamera::OpenConnection(){

	bool bRet = false;
	//创建套接字
	m_iSockConn = socket(AF_INET, SOCK_STREAM, 0);

	//向服务器（特定的IP和端口）发起请求
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
	serv_addr.sin_family = AF_INET;  //使用IPv4地址
	serv_addr.sin_addr.s_addr = inet_addr(m_arrRemoteAddr);  //具体的IP地址
	serv_addr.sin_port = htons(m_iRemotePort);  //端口
	if ( 0 != connect(m_iSockConn, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ){
		LogUtil.Log(CLogUtil::eWTF, "connect receiver failed. return");
		return bRet;
	}
	else{
		bRet = true;
	}
	return bRet;
}

void CVideoCamera::Start(){

	if (OpenConnection()){
		std::thread t(&CVideoCamera::Encode, this);
		t.detach();
	}
}
