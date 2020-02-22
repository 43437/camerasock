/*
 * CLogUtil.h
 *
 *  Created on: Feb 16, 2020
 *      Author: acmax
 */

#ifndef CLOGUTIL_H_
#define CLOGUTIL_H_

#include <mutex>
#define LogUtil CLogUtil::GetInstance()

class CLogUtil {
public:
	CLogUtil();
	virtual ~CLogUtil();
	static CLogUtil& GetInstance();

	void Log(int iLogLevel, const char* szFormat, ...);

public:
	enum ELogLevel{
		eDebug,
		eInfo,
		eError,
		eWTF,
	};

private:
	void AddLogLevelInfo(int iLogLevel, int& iLogStartIndex);

private:
	enum{
		eLogBuffSize = 2048,
	};
	std::mutex m_LogMutex;
	int m_iLogLevel;
	char m_arrLogBuff[eLogBuffSize];
};

#endif /* CLOGUTIL_H_ */
