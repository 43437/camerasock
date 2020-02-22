/*
 * CVideoReceiver.h
 *
 *  Created on: Feb 9, 2020
 *      Author: acmax
 */

#ifndef CVIDEORECEIVER_H_
#define CVIDEORECEIVER_H_

#include <iostream>
#include "CDataPool.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
}

#define REFRESH_EVENT  (SDL_USEREVENT + 1)

class CVideoReceiver {
public:
	CVideoReceiver();
	virtual ~CVideoReceiver();
	void Start();
	void Terminal(){m_bStop = true;};
	uint8_t *GetBuffer();

private:
	void Decode(uint8_t* pBuff, const int& iLen);
	void Receive();
	int InitContext(AVCodecContext *pSDLCodecCtx);

private:
	uint8_t *m_pSDLFrameBuff;
	const int m_iSDLBufSize;
	bool m_bStop;
	const char* m_arrLocalAddr;
	const int m_iLocalPort;
};

#endif /* CVIDEORECEIVER_H_ */
