/*
 * CDataPool.h
 *
 *  Created on: Feb 9, 2020
 *      Author: acmax
 */

#ifndef CDATAPOOL_H_
#define CDATAPOOL_H_

#include <iostream>
#include <string>
#include <mutex>

#include "CVideoCamera.h"

extern "C"{
#include <libavcodec/avcodec.h>
}

struct SYUVData{
	enum{
		ePacketDataSize = 1024000,	//串行化Packet数据字节最大存储空间
	};
	AVPacket m_stuPacket;
	bool m_bIsValid;
	SYUVData();
};

class CDataPool {
public:
	CDataPool();
	virtual ~CDataPool();
	void Init();

	void AppendBufferData(const uint8_t* buf, int iSize);
	bool TakeNextPacketData(AVPacket*& pPacket);
	void DisardNextPacketData();
//	void ClearPacketListData();
	static CDataPool& GetInstance();

private:
	void Slice();
	int FindNextPacketBegin();
	int FindNextPacketEnd();
	int FindSplitFlag(const char* splitFlag);
	//iIndex负值代表清空整个
	void DiscardBufferData(int iIndex = -1);
	void SaveDataToPacketList(int iIndexBeg, int iIndexEnd);
	bool ConvertBufferToYUVPacket(const int iIndexBeg, const int iIndexEnd, SYUVData* pYUVData);
	int GetNextPacketIndex(int iIndex);
	int GetNextReadPacketIndex();
	int GetNextWritePacketIndex();
	void UpdatePacketListHead(int iIndex);
	void UpdatePacketListTail(int iIndex);
	bool PacketFromBuffer(AVPacket& stuPktRcv, uint8_t* pData, int iDataSize);

private:
	uint8_t* m_pCache;
	int m_iCacheSize;			//总长度
	int m_iCurrSize;			//当前已经存入的长度
//	std::mutex m_PacktListMutex;
	enum{
		ePacketListSize = 10,		//串行化Packet链表长度
	};
	SYUVData* m_arrPacketList[ePacketListSize];	//串行化Packet链表
	int m_iPacketListHead;			//串行化Packet链表头部，下一读位置
	int m_iPacketListTail;			//串行化Packet链表尾部，下一写位置
};

#endif /* CDATAPOOL_H_ */
