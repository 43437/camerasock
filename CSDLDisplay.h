/*
 * CSDLDisplay.h
 *
 *  Created on: Feb 18, 2020
 *      Author: acmax
 */

#ifndef CSDLDISPLAY_H_
#define CSDLDISPLAY_H_

#include "CVideoReceiver.h"

extern "C"{
#include <SDL2/SDL.h>
}

class CSDLDisplay {
public:
	CSDLDisplay(CVideoReceiver& stuVideoReceiver);
	virtual ~CSDLDisplay();
	int Render();
private:
	CVideoReceiver& m_stuVideoReceiver;
	int m_iScreenW;	//SDL窗口初始宽
	int m_iScreenH;	//SDL窗口初始高
};

#endif /* CSDLDISPLAY_H_ */
