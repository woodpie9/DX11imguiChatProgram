#pragma once
#include "CommonDefines.h"
#include <cstdlib>					// C 표준 유틸리티 함수들을 모아놓은 해더파일

// 202328 코드 스타일 변경
// 202310 코드에 주석 작성 완료
// 230309 woodpie9 코드 생성
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

	StreamQueue(const int size) : queue_size_(size)
	{
		buffer_ = static_cast<char*>(malloc(size));
		Clear();
	}
	~StreamQueue()
	{
		free(buffer_);
	}

	void Clear();

	bool IsEmpty() const;
	bool IsFull() const;

	int Remain() const;
	int Size() const;
	int Count() const;

	bool Peek(char* peek_buf, int peek_len) const;
	int Read(char* des_buf, int buf_len);
	int Write(const char* src_data, int bytes_data);

private:
	short queue_size_;			// 환영 큐의 크기
	short data_count_;			// 큐에 있는 데이터의 갯수
	short read_index_;			// 읽을 데이터가 있는 위치
	short write_index_;			// 데이터를 쓸 수있는 위치
	char* buffer_;				// 데이터의 배열의 포인터

};



WOODNET_END