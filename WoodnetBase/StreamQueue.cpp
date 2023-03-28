#include "StreamQueue.h"
#include <algorithm>

void woodnet::StreamQueue::Clear()
{
	// ������� ���۸� 0���� �ʱ�ȭ�մϴ�.
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
	// ��� ������ ������ ����Ʈ ���� �����մϴ�.
	return queue_size_ - data_count_;
}

int woodnet::StreamQueue::Size() const
{
	return queue_size_;
}

int woodnet::StreamQueue::Count() const
{
	// ���� ť�� ����� ����Ʈ ���� ��ȯ�մϴ�.
	return data_count_;
}

bool woodnet::StreamQueue::Peek(char* peek_buf, int peek_len) const
{
	// ť���� �����͸� �������� �ʰ� �����͸� �����մϴ�.
	// ť�� �ִ� ũ�⺸�� �� ���� peek �� ���� �����ϴ�.
	if (peek_len > Count()) return false;

	// ���� �ε����� �� ũ�� ó������ �б� �ε������� �����մϴ�.
	if (read_index_ < write_index_)
	{
		memcpy(peek_buf, &buffer_[read_index_], peek_len);
	}
	else
	{
		// �б� �ε����κ��� ť�� �������� ũ�⸦ ���մϴ�.
		const int back_part = queue_size_ - read_index_;

		// �д� ������ ť�� �� ���� ª�ٸ� �ٷ� �н��ϴ�.
		if (peek_len <= back_part)
		{
			memcpy(peek_buf, &buffer_[read_index_], peek_len);
		}
		else
		{
			// ť�� ������ ���� �� ���� peek ���̸� ���մϴ�.
			const int fore_part = peek_len - back_part;

			// ť�� ���������� ���� ��, ť�� ó������ ���� ���̸�ŭ �н��ϴ�.
			memcpy(&peek_buf[0], &buffer_[read_index_], back_part);
			memcpy(&peek_buf[back_part], &buffer_[0], fore_part);
		}
	}

	return true;
}

int woodnet::StreamQueue::Read(char* des_buf, int buf_len)
{
	// ť���� �����͸� �о�ɴϴ�.

	// ť�� ����ִٸ� ���� �� �����ϴ�.
	if (IsEmpty()) return 0;

	// ���� ���� �ִ� ������ ���� or ���ϴ� ������ �� ���� �߿� ���� ������ŭ �н��ϴ�.
	const int read_count = std::min<short>(data_count_, buf_len);

	// �б� �ε����� ���� �ε������� �ڿ� ������ �б� �ε������� �н��ϴ�.
	if (read_index_ < write_index_)
	{
		memcpy(des_buf, &buffer_[read_index_], read_count);
	}
	else
	{
		// �б� �ε����κ��� ť�� �������� ũ�⸦ ���մϴ�.
		int back_part = queue_size_ - read_index_;

		// �д� ������ ť�� ������ ª�ٸ� �ٷ� �н��ϴ�.
		if (read_count <= back_part)
		{
			memcpy(des_buf, &buffer_[read_index_], read_count);
		}
		else
		{
			// ť�� ������ ���� �� ���� ���̸� ���մϴ�.
			int fore_part = read_count - back_part;

			// ť�� ���������� ���� ��, ť�� ó������ ���� ���̸�ŭ �н��ϴ�.
			memcpy(&des_buf[0], &buffer_[read_index_], back_part);
			memcpy(&des_buf[back_part], &buffer_[0], fore_part);
		}
	}

	data_count_ -= read_count;		// ���� ������ŭ �� ������ �������� ���ϴ�.
	read_index_ += read_count;		// ���� ������ŭ �б� �ε����� �̵��մϴ�.

	if (read_index_ >= queue_size_)		// �б� �ε����� �ùٸ� ��ġ�� �̵���ŵ�ϴ�.
		read_index_ -= queue_size_;

	return read_count;
}

int woodnet::StreamQueue::Write(const char* src_data, int bytes_data)
{
	// ť�� �����͸� �ۼ��մϴ�.

	// ť�� ���� ���ִٸ� �ۼ��� �� �����ϴ�.
	if (IsFull()) return 0;

	// ť�� �����ִ� ĭ ���� �ۼ��ϰ� ���� �������� ���� �� ���� �� ��ŭ �ۼ��մϴ�.
	const int left = Remain();
	const int write_count = std::min<short>(left, bytes_data);

	// ���� �ε����� �б� �ε������� �ڿ� ������ ���� �ε������� ���ϴ�.
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
