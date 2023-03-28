#pragma once
#include "CommonDefines.h"
#include <cstdlib>					// C ǥ�� ��ƿ��Ƽ �Լ����� ��Ƴ��� �ش�����

// 202328 �ڵ� ��Ÿ�� ����
// 202310 �ڵ忡 �ּ� �ۼ� �Ϸ�
// 230309 woodpie9 �ڵ� ����
// TCP ��Ʈ��ũ �����ʹ� ����Ʈ��Ʈ������ �ٷ��.
// ȯ�� ť�� ����Ʈ��Ʈ���� �����Ѵ�.

WOODNET_BEGIN

class StreamQueue
{
public:
	// ������ �ʴ� �����ڵ��� �����Ѵ�.
	StreamQueue() = delete;
	StreamQueue(const StreamQueue&) = delete;
	StreamQueue& operator=(const StreamQueue&) = delete;		// ��������� ����

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
	short queue_size_;			// ȯ�� ť�� ũ��
	short data_count_;			// ť�� �ִ� �������� ����
	short read_index_;			// ���� �����Ͱ� �ִ� ��ġ
	short write_index_;			// �����͸� �� ���ִ� ��ġ
	char* buffer_;				// �������� �迭�� ������

};



WOODNET_END