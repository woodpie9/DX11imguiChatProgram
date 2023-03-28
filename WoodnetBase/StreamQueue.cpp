#include "StreamQueue.h"
#include <algorithm>

void woodnet::StreamQueue::Clear()
{
	// 변수들과 버퍼를 0으로 초기화합니다.
	data_count_ = read_index_ = write_index_ = 0;
	memset(buffer_, 0, queue_size_);
}

bool woodnet::StreamQueue::IsEmpty() const
{
	return data_count_ == 0 ? true : false;
}

bool woodnet::StreamQueue::IsFull() const
{
	return queue_size_ == data_count_ ? true : false;
}

int woodnet::StreamQueue::Remain() const
{
	// 사용 가능한 버퍼의 바이트 수를 리턴합니다.
	return queue_size_ - data_count_;
}

int woodnet::StreamQueue::Size() const
{
	return queue_size_;
}

int woodnet::StreamQueue::Count() const
{
	// 현재 큐에 저장된 바이트 수를 반환합니다.
	return data_count_;
}

bool woodnet::StreamQueue::Peek(char* peek_buf, int peek_len) const
{
	// 큐에서 데이터를 제거하지 않고 데이터를 복사합니다.
	// 큐의 최대 크기보다 더 많이 peek 할 수는 없습니다.
	if (peek_len > Count()) return false;

	// 쓰기 인덱스가 더 크면 처음부터 읽기 인덱스까지 복사합니다.
	if (read_index_ < write_index_)
	{
		memcpy(peek_buf, &buffer_[read_index_], peek_len);
	}
	else
	{
		// 읽기 인덱스로부터 큐의 끝까지의 크기를 구합니다.
		const int back_part = queue_size_ - read_index_;

		// 읽는 범위가 큐의 끝 보다 짧다면 바로 읽습니다.
		if (peek_len <= back_part)
		{
			memcpy(peek_buf, &buffer_[read_index_], peek_len);
		}
		else
		{
			// 큐의 끝까지 읽은 후 남은 peek 길이를 구합니다.
			const int fore_part = peek_len - back_part;

			// 큐의 마지막까지 읽은 후, 큐의 처음부터 남은 길이만큼 읽습니다.
			memcpy(&peek_buf[0], &buffer_[read_index_], back_part);
			memcpy(&peek_buf[back_part], &buffer_[0], fore_part);
		}
	}

	return true;
}

int woodnet::StreamQueue::Read(char* des_buf, int buf_len)
{
	// 큐에서 데이터를 읽어옵니다.

	// 큐가 비어있다면 읽을 수 없습니다.
	if (IsEmpty()) return 0;

	// 버퍼 내에 있는 데이터 갯수 or 원하는 데이터 의 갯수 중에 작은 갯수만큼 읽습니다.
	const int read_count = std::min<short>(data_count_, buf_len);

	// 읽기 인덱스가 쓰기 인덱스보다 뒤에 있으면 읽기 인덱스부터 읽습니다.
	if (read_index_ < write_index_)
	{
		memcpy(des_buf, &buffer_[read_index_], read_count);
	}
	else
	{
		// 읽기 인덱스로부터 큐의 끝까지의 크기를 구합니다.
		int back_part = queue_size_ - read_index_;

		// 읽는 범위가 큐의 끝보다 짧다면 바로 읽습니다.
		if (read_count <= back_part)
		{
			memcpy(des_buf, &buffer_[read_index_], read_count);
		}
		else
		{
			// 큐의 끝까지 읽은 후 남은 길이를 구합니다.
			int fore_part = read_count - back_part;

			// 큐의 마지막까지 읽은 후, 큐의 처음부터 남은 길이만큼 읽습니다.
			memcpy(&des_buf[0], &buffer_[read_index_], back_part);
			memcpy(&des_buf[back_part], &buffer_[0], fore_part);
		}
	}

	data_count_ -= read_count;		// 읽은 갯수만큼 총 데이터 갯수에서 뺍니다.
	read_index_ += read_count;		// 읽은 갯수만큼 읽기 인덱스를 이동합니다.

	if (read_index_ >= queue_size_)		// 읽기 인덱스를 올바른 위치로 이동시킵니다.
		read_index_ -= queue_size_;

	return read_count;
}

int woodnet::StreamQueue::Write(const char* src_data, int bytes_data)
{
	// 큐에 데이터를 작성합니다.

	// 큐가 가득 차있다면 작성할 수 없습니다.
	if (IsFull()) return 0;

	// 큐의 남아있는 칸 수와 작성하고 싶은 데이터의 갯수 중 작은 수 만큼 작성합니다.
	const int left = Remain();
	const int write_count = std::min<short>(left, bytes_data);

	// 쓰기 인덱스가 읽기 인덱스보다 뒤에 있으면 쓰기 인덱스부터 씁니다.
	if (read_index_ > write_index_)
	{
		memcpy(&buffer_[write_index_], src_data, write_count);
	}
	else
	{
		const int back_space = queue_size_ - write_index_;

		// 
		if (back_space >= write_count)
		{
			memcpy(&buffer_[write_index_], src_data, write_count);
		}
		else
		{
			const int fore_space = write_count - back_space;
			memcpy(&buffer_[write_index_], &src_data[0], back_space);
			memcpy(&buffer_[0], &src_data[back_space], fore_space);
		}
	}

	data_count_ += write_count;
	write_index_ += write_count;

	if (write_index_ >= queue_size_)
		write_index_ -= queue_size_;

	return write_count;
}
