/*
 * CLogUtil.cpp
 *
 *  Created on: Feb 16, 2020
 *      Author: acmax
 */

#include "CLogUtil.h"
#include <cstring>
#include <iostream>

extern "C"{
#include <stdarg.h>
#include <stdio.h>
}

CLogUtil::CLogUtil():
		m_iLogLevel(eWTF){
	// TODO Auto-generated constructor stub

}

CLogUtil::~CLogUtil() {
	// TODO Auto-generated destructor stub
}

CLogUtil& CLogUtil::GetInstance(){
	static CLogUtil stuLogUtil;
	return stuLogUtil;
}

void CLogUtil::Log(int iLogLevel, const char* szFormat, ...){

	if ( iLogLevel >= m_iLogLevel ){
		int iLogStartIndex = 0;
		AddLogLevelInfo(iLogLevel, iLogStartIndex);
		va_list arglist;
		va_start(arglist, szFormat);
		vsprintf(&m_arrLogBuff[iLogStartIndex] ,szFormat, arglist);
		va_end(arglist);

		std::lock_guard<std::mutex> guard(m_LogMutex);
		std::cout<<m_arrLogBuff<<std::endl;
	}
	else{
		;	//不需要记日志
	}
}

void CLogUtil::AddLogLevelInfo(int iLogLevel, int& iLogStartIndex){

	char const *pLogTag = NULL;
	int iLogTagLen = 0;
	switch(iLogLevel){
	case eDebug:
		pLogTag = "DEBUG: ";
		iLogTagLen = 7;
		break;
	case eInfo:
		pLogTag = "INFO: ";
		iLogTagLen = 6;
		break;
	case eError:
		pLogTag = "ERROR: ";
		iLogTagLen = 7;
		break;
	case eWTF:
		pLogTag = "WTF: ";
		iLogTagLen = 5;
		break;
	default:
		break;
	}
	if (iLogTagLen > 0 ){
		memcpy(&m_arrLogBuff[0], pLogTag, iLogTagLen);
		iLogStartIndex = iLogTagLen;
	}
}
