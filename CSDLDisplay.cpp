/*
 * CSDLDisplay.cpp
 *
 *  Created on: Feb 18, 2020
 *      Author: acmax
 */

#include "CSDLDisplay.h"
#include "CLogUtil.h"

CSDLDisplay::CSDLDisplay(CVideoReceiver& stuVideoReceiver):
							m_stuVideoReceiver(stuVideoReceiver),
							m_iScreenW(852),
							m_iScreenH(480){
	// TODO Auto-generated constructor stub

}

CSDLDisplay::~CSDLDisplay() {
	// TODO Auto-generated destructor stub
}

int CSDLDisplay::Render(){

	SDL_Event event;
	SDL_Rect stuSDLRect;
	stuSDLRect.x = 0;
	stuSDLRect.y = 0;
	stuSDLRect.w = m_iScreenW;
	stuSDLRect.h = m_iScreenH;

	SDL_Window *pSDLScreen;	//SDL窗口句柄
	SDL_Renderer* pSDLRenderer;	//SDL渲染器
	SDL_Texture* pSDLTexture;	//SDL纹理

	if(SDL_Init(SDL_INIT_VIDEO)){
			LogUtil.Log(CLogUtil::eWTF, "Could not initialize SDL - %s", SDL_GetError());
			return -1;
	}

	pSDLScreen = SDL_CreateWindow("Simplest Video Play SDL2"
									, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
									, m_iScreenW, m_iScreenH
									, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if (NULL == pSDLScreen){
		LogUtil.Log(CLogUtil::eWTF, "SDL_CreateWindow failed. ");
		return -1;
	}

	pSDLRenderer = SDL_CreateRenderer(pSDLScreen, -1, 0);
	if (NULL == pSDLRenderer){
		LogUtil.Log(CLogUtil::eWTF, "SDL_CreateRenderer failed. ");
		return -1;
	}

	pSDLTexture = SDL_CreateTexture(pSDLRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 640, 360);
	if (NULL == pSDLTexture){
		LogUtil.Log(CLogUtil::eWTF, "SDL_CreateTexture failed.");
		return -1;
	}

	while(1){
		//等待事件
		SDL_WaitEvent(&event);
		if(event.type == REFRESH_EVENT){
			//使用最新window窗口大小显示
			stuSDLRect.x = 0;
			stuSDLRect.y = 0;
			stuSDLRect.w = m_iScreenW;
			stuSDLRect.h = m_iScreenH;

			SDL_UpdateTexture( pSDLTexture, NULL, m_stuVideoReceiver.GetBuffer(), PIXEL_W);
			SDL_RenderClear( pSDLRenderer );
			SDL_RenderCopy( pSDLRenderer, pSDLTexture, NULL, &stuSDLRect);
			SDL_RenderPresent( pSDLRenderer );
			//Delay 40ms
//			SDL_Delay(40);

		}
		else if(event.type == SDL_WINDOWEVENT){
			//窗口大小改变
			SDL_GetWindowSize(pSDLScreen, &m_iScreenW, &m_iScreenH);
		}
		else if(event.type == SDL_QUIT){
			break;
		}
	}

	return 0;
}

