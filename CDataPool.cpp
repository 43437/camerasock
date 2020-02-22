/*
 * CDataPool.cpp
 *
 *  Created on: Feb 9, 2020
 *      Author: acmax
 */

#include "CDataPool.h"
#include <cstring>
#include "CLogUtil.h"

extern "C"{
#include <malloc.h>
}

SYUVData::SYUVData():
m_bIsValid(false){
	memset(&m_stuPacket, 0, sizeof(AVPacket));
	av_init_packet(&m_stuPacket);
	m_stuPacket.data = (uint8_t*)malloc(ePacketDataSize);
	if (NULL == m_stuPacket.data){
		LogUtil.Log(CLogUtil::eWTF, "SYUVData packet data malloc failed. ");
	}
}
CDataPool::CDataPool():
		m_iCacheSize(102400),
		m_iCurrSize(0),
		m_iPacketListHead(0),
		m_iPacketListTail(0)
{
	// TODO Auto-generated constructor stub
	Init();
}

void CDataPool::Init(){

	m_pCache = (uint8_t*)malloc(m_iCacheSize);
	if (NULL == m_pCache){
		LogUtil.Log(CLogUtil::eWTF, "Init m_pCache failed. ");
	}

	for (int i = 0; i < ePacketListSize; ++i){
		SYUVData* pYUVData = new SYUVData();
		m_arrPacketList[i] = pYUVData;
	}
}

CDataPool::~CDataPool() {
	// TODO Auto-generated destructor stub
	free(m_pCache);
	for (int i = 0; i < ePacketListSize; ++i){
		free( m_arrPacketList[i] );
	}
}

void CDataPool::AppendBufferData(const uint8_t* buf, int iSize) {

	//当buffer未分配时，申请iSize * 2的空间
	if (NULL == m_pCache){
		m_iCacheSize = iSize * 2;
		m_pCache = (uint8_t*)malloc(m_iCacheSize);
	}
	if (NULL == m_pCache){
		m_iCacheSize = 0;
		LogUtil.Log(CLogUtil::eWTF, "append malloc error.");
	}

	//当buffer空间不足时，增加iSize * 2的空间
	if ( (m_iCacheSize - m_iCurrSize) < iSize ){

		int iIncrease = iSize * 2;
		uint8_t* pBufTmp = (uint8_t*)realloc(m_pCache, m_iCacheSize + iIncrease);
		if ( NULL != pBufTmp ){
			m_iCacheSize += iIncrease;
			m_pCache = pBufTmp;
		}
		else{
			LogUtil.Log(CLogUtil::eWTF, "realloc failed. ");
			return;
		}
	}
	memcpy(&m_pCache[m_iCurrSize], buf, iSize);
	m_iCurrSize += iSize;

	Slice();
}

void CDataPool::Slice(){

	int iBegin = FindNextPacketBegin();
	int iEnd = FindNextPacketEnd();
	if ( iEnd > 0){

		if ( iEnd <= iBegin ){
			LogUtil.Log(CLogUtil::eWTF, "slice end before begin.");
			int iDiscardIndex = iBegin;
			DiscardBufferData(iDiscardIndex);
		}
		else{
			if ( iBegin < 0 ){
				LogUtil.Log(CLogUtil::eWTF, "iBegin less 0.");
				int iDescardIndex = iEnd + DATA_SPLIT_LEN;
				DiscardBufferData(iDescardIndex);
			}
			else{
				if ( iBegin > 0 ){
					LogUtil.Log(CLogUtil::eWTF, "iBegin lt 0 [%d]", iBegin);
					int iDiscardIndex = iBegin;
					DiscardBufferData(iDiscardIndex);
				}

				//把收到的一个完整的AVPacket保存
				iBegin = FindNextPacketBegin();
				iEnd = FindNextPacketEnd();
				LogUtil.Log(CLogUtil::eDebug, "iBegin[%d] iEnd[%d]", iBegin, iEnd);
				if (iBegin < iEnd && iBegin == 0){
					SaveDataToPacketList(iBegin, iEnd);
				}
				else{
					LogUtil.Log(CLogUtil::eWTF, "after DiscardBufferData also wrong");
				}
			}
		}
	}
	else{
		//还没有结束继续
		LogUtil.Log(CLogUtil::eDebug, "Slice not end.");
	}
}

int CDataPool::FindNextPacketBegin(){

	return FindSplitFlag(BEGIN_FLAG);
}

int CDataPool::FindSplitFlag(const char* splitFlag){

	int iIndex = -1;
	if (NULL != splitFlag){
		//最后一个字节要填充0
		char arrSplit[DATA_SPLIT_LEN+1] = {0};
		//todo找到分割的开始
		for (int i = 0; ( i + DATA_SPLIT_LEN ) <= m_iCurrSize; ++i){

			memcpy(arrSplit, &m_pCache[i], DATA_SPLIT_LEN);
			if (0 == strcmp(arrSplit, splitFlag)){
				//找到了退出
				iIndex = i;
				break;
			}
		}
	}

	return iIndex;
}

int CDataPool::FindNextPacketEnd(){

	return FindSplitFlag(END_FLAG);
}

void CDataPool::DiscardBufferData(const int iIndex){

	if ( iIndex < 0 ){
		//丢弃所有数据
		memset(m_pCache, 0, m_iCacheSize);
		m_iCurrSize = 0;
	}
	else{
		if (iIndex < m_iCurrSize ){
			memset(m_pCache, 0, iIndex);
			//拷贝数据到起始位置
			m_iCurrSize = m_iCurrSize - iIndex;
			memmove(m_pCache, &m_pCache[iIndex], m_iCurrSize);
		}else{
			if (iIndex == m_iCurrSize){
				memset(m_pCache, 0, m_iCurrSize);
				m_iCurrSize = 0;
			}else{
				//超出当前有效数据长度范围，清空整个缓存区
				memset(m_pCache, 0, m_iCacheSize);
				m_iCurrSize = 0;
				LogUtil.Log(CLogUtil::eWTF, "index oversize m_iCurrSize.");
			}
		}
	}
}

void CDataPool::SaveDataToPacketList(int iIndexBeg, int iIndexEnd){

//	std::lock_guard<std::mutex> guard(m_PacktListMutex);
	int iIndex = GetNextWritePacketIndex();
	if ( iIndex != -1 ){
		bool bRet = ConvertBufferToYUVPacket(iIndexBeg, iIndexEnd, m_arrPacketList[iIndex]);
		m_arrPacketList[iIndex]->m_bIsValid = bRet;
		if ( bRet ){
			int iTail = GetNextPacketIndex(iIndex);
			UpdatePacketListTail(iTail);
		}
		LogUtil.Log(CLogUtil::eDebug, "SaveDataToPacketList ret:[%d] =? [%d]", bRet, true);
	}
}

bool CDataPool::ConvertBufferToYUVPacket(const int iIndexBeg, const int iIndexEnd, SYUVData* pYUVData){

	bool bRet = false;
	const int iBegin = iIndexBeg + DATA_SPLIT_LEN;
	const int iEnd = iIndexEnd;
	int iLen = iEnd - iBegin;
	if ( iLen > 0 ){
		if ( iEnd < m_iCurrSize ){
			//保存数据
			bRet = PacketFromBuffer(pYUVData->m_stuPacket, &m_pCache[iBegin], iLen);
			LogUtil.Log(CLogUtil::eDebug, "PacketFromBuffer ret:[%d] =? [%d]", bRet, true);
		}
		else{
			LogUtil.Log(CLogUtil::eWTF, "ConvertBufferToYUVPacket index out off");
		}

		const int iDiscardIndex = iIndexEnd + DATA_SPLIT_LEN;
		DiscardBufferData(iDiscardIndex);
	}
	else{
		LogUtil.Log(CLogUtil::eWTF, "iLen negative.");
	}
	return bRet;
}

bool CDataPool::PacketFromBuffer(AVPacket& stuPktRcv, uint8_t* pData, int iDataSize){

	bool bRet = false;
	int iPacketDataLen = sizeof(stuPktRcv.pts) + sizeof(stuPktRcv.dts) + sizeof(stuPktRcv.stream_index) + sizeof(stuPktRcv.flags) + sizeof(stuPktRcv.size);
	if (iPacketDataLen < iDataSize){
		int iIndex = 0;
		memcpy( &stuPktRcv.pts, &pData[iIndex], sizeof(stuPktRcv.pts));	//pts
		iIndex += sizeof(stuPktRcv.pts);
		memcpy( &stuPktRcv.dts, &pData[iIndex], sizeof(stuPktRcv.dts));	//dts
		iIndex += sizeof(stuPktRcv.dts);
		memcpy(&stuPktRcv.stream_index, &pData[iIndex], sizeof(stuPktRcv.stream_index));	//stream_index
		iIndex += sizeof(stuPktRcv.stream_index);
		memcpy( &stuPktRcv.flags, &pData[iIndex], sizeof(stuPktRcv.flags));	//flags
		iIndex += sizeof(stuPktRcv.flags);
		memcpy( &stuPktRcv.size, &pData[iIndex], sizeof(stuPktRcv.size));	//size
		iIndex += sizeof(stuPktRcv.size);
		LogUtil.Log(CLogUtil::eDebug, "PacketFromBuffer size:[%d]", stuPktRcv.size);

		if (iIndex + stuPktRcv.size <= iDataSize){
			memcpy( stuPktRcv.data, &pData[iIndex], stuPktRcv.size);	//data
			bRet = true;
		}
		else{
			LogUtil.Log(CLogUtil::eWTF, "PacketFromBuffer data outoff.");
		}
	}
	else{
		LogUtil.Log(CLogUtil::eWTF, "PacketFromBuffer data len incorrect. ");
	}

	return bRet;
}

int CDataPool::GetNextPacketIndex(int iIndex){

	return (iIndex + 1) % ePacketListSize;
}

int CDataPool::GetNextReadPacketIndex(){

	int iNextIndex = -1;
	//下一个位置的数据是有效的就可以读
	if ( m_arrPacketList[m_iPacketListHead]->m_bIsValid){
		iNextIndex = m_iPacketListHead;
	}

	LogUtil.Log(CLogUtil::eDebug, "head:[%d] tail:[%d]", m_iPacketListHead, m_iPacketListTail);
	return iNextIndex;
}

int CDataPool::GetNextWritePacketIndex(){

	int iNextIndex = -1;
	//下一个位置的数据是无效的就可以写
	if ( ! m_arrPacketList[m_iPacketListTail]->m_bIsValid ){
		iNextIndex = m_iPacketListTail;
	}

	LogUtil.Log(CLogUtil::eDebug, "head:[%d] tail:[%d]", m_iPacketListHead, m_iPacketListTail);
	return iNextIndex;
}

void CDataPool::UpdatePacketListHead(int iIndex){

	m_iPacketListHead = iIndex;
}

void CDataPool::UpdatePacketListTail(int iIndex){

	m_iPacketListTail = iIndex;
}

bool CDataPool::TakeNextPacketData(AVPacket*& pPacket){

	bool bRet = false;
	int iIndex = GetNextReadPacketIndex();
	if ( -1 != iIndex ){
		pPacket = &(m_arrPacketList[iIndex]->m_stuPacket);
		bRet = true;
	}
	else{

	}

	return bRet;
}

void CDataPool::DisardNextPacketData(){

	m_arrPacketList[m_iPacketListHead]->m_bIsValid = false;
	int iIndex = GetNextPacketIndex(m_iPacketListHead);
	UpdatePacketListHead(iIndex);
}

CDataPool& CDataPool::GetInstance(){
	static CDataPool stuInstance;
	return stuInstance;
}
