// ReSharper disable CppClangTidyClangDiagnosticInvalidUtf8
#pragma once
#include "CommonDefines.h"
#include <cstdlib>


// 230309 woodpie9 코드 생성, 주석 작성중
// TCP 네트워크 데이터는 바이트스트림으로 다룬다.
// 환영 큐로 바이트스트림을 구현한다.

WOODNET_BEGIN

class StreamQueue
{
public:
	// 원하지 않는 생성자들을 삭제한다.
	StreamQueue() = delete;
	StreamQueue(const StreamQueue&) = delete;
	StreamQueue& operator=(const StreamQueue&) = delete;		// 복사생성자 삭제

	StreamQueue(const int size)
	{
		m_size = size;
		m_buffer = static_cast<char*>(malloc(size));
		clear();
	}
	~StreamQueue()
	{
		free(m_buffer);
	}

	void clear();

	bool is_empty() const;
	bool is_full() const;

	int remain() const;
	int size() const;
	int count() const;

	bool peek(char* peekBuf, int peekLen) const;
	int read(char* desBuf, int bufLen);
	int write(const char* srcData, int bytesData);

private:
	int m_size;					// 환영 큐의 크기
	short m_dataCount;			// 큐에 있는 데이터의 갯수
	short m_readIndex;			// 읽을 데이터가 있는 위치
	short m_writeIndex;			// 데이터를 쓸 수있는 위치
	char* m_buffer;				// 데이터의 배열의 포인터

};



WOODNET_END