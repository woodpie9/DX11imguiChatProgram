#include "StreamQueue.h"
#include <algorithm>

void woodnet::StreamQueue::clear()
{
	// 변수들과 버퍼를 0으로 초기화합니다.
	m_dataCount = m_readIndex = m_writeIndex = 0;
	memset(m_buffer, 0, m_size);
}

bool woodnet::StreamQueue::is_empty() const
{
	return m_dataCount == 0 ? true : false;
}

bool woodnet::StreamQueue::is_full() const
{
	return m_size == m_dataCount ? true : false;
}

int woodnet::StreamQueue::remain() const
{
	// 사용 가능한 버퍼의 바이트 수를 리턴합니다.
	return m_size - m_dataCount;
}

int woodnet::StreamQueue::size() const
{
	return m_size;
}

int woodnet::StreamQueue::count() const
{
	// 현재 큐에 저장된 바이트 수를 반환합니다.
	return m_dataCount;
}

bool woodnet::StreamQueue::peek(char* peekBuf, int peekLen) const
{
	// 큐에서 데이터를 제거하지 않고 데이터를 복사합니다.
	// 큐의 최대 크기보다 더 많이 peek 할 수는 없습니다.
	if (peekLen > count()) return false;

	// 쓰기 인덱스가 더 크면 처음부터 읽기 인덱스까지 복사합니다.
	if (m_readIndex < m_writeIndex)
	{
		memcpy(peekBuf, &m_buffer[m_readIndex], peekLen);
	}
	else
	{
		// 읽기 인덱스로부터 큐의 끝까지의 크기를 구합니다.
		const int back_part = m_size - m_readIndex;

		// 읽는 범위가 큐의 끝 보다 짧다면 바로 읽습니다.
		if (peekLen <= back_part)
		{
			memcpy(peekBuf, &m_buffer[m_readIndex], peekLen);
		}
		else
		{
			// 큐의 끝까지 읽은 후 남은 peek 길이를 구합니다.
			const int fore_part = peekLen - back_part;

			// 큐의 마지막까지 읽은 후, 큐의 처음부터 남은 길이만큼 읽습니다.
			memcpy(&peekBuf[0], &m_buffer[m_readIndex], back_part);
			memcpy(&peekBuf[back_part], &m_buffer[0], fore_part);
		}
	}

	return true;
}

int woodnet::StreamQueue::read(char* desBuf, int bufLen)
{
	// 큐에서 데이터를 읽어옵니다.

	// 큐가 비어있다면 읽을 수 없습니다.
	if (is_empty()) return 0;

	// 버퍼 내에 있는 데이터 갯수 or 원하는 데이터 의 갯수 중에 작은 갯수만큼 읽습니다.
	const int read_count = std::min<short>(m_dataCount, bufLen);

	// 읽기 인덱스가 쓰기 인덱스보다 뒤에 있으면 읽기 인덱스부터 읽습니다.
	if (m_readIndex < m_writeIndex)
	{
		memcpy(desBuf, &m_buffer[m_readIndex], read_count);
	}
	else
	{
		// 읽기 인덱스로부터 큐의 끝까지의 크기를 구합니다.
		int back_part = m_size - m_readIndex;

		// 읽는 범위가 큐의 끝보다 짧다면 바로 읽습니다.
		if (read_count <= back_part)
		{
			memcpy(desBuf, &m_buffer[m_readIndex], read_count);
		}
		else
		{
			// 큐의 끝까지 읽은 후 남은 길이를 구합니다.
			int fore_part = read_count - back_part;

			// 큐의 마지막까지 읽은 후, 큐의 처음부터 남은 길이만큼 읽습니다.
			memcpy(&desBuf[0], &m_buffer[m_readIndex], back_part);
			memcpy(&desBuf[back_part], &m_buffer[0], fore_part);
		}
	}

	m_dataCount -= read_count;		// 읽은 갯수만큼 총 데이터 갯수에서 뺍니다.
	m_readIndex += read_count;		// 읽은 갯수만큼 읽기 인덱스를 이동합니다.

	if (m_readIndex >= m_size)		// 읽기 인덱스를 올바른 위치로 이동시킵니다.
		m_readIndex -= m_size;

	return read_count;
}

int woodnet::StreamQueue::write(const char* srcData, int bytesData)
{
	// 큐에 데이터를 작성합니다.

	// 큐가 가득 차있다면 작성할 수 없습니다.
	if (is_full()) return 0;

	// 큐의 남아있는 칸 수와 작성하고 싶은 데이터의 갯수 중 작은 수 만큼 작성합니다.
	const int left = remain();
	const int write_count = std::min<short>(left, bytesData);

	// 쓰기 인덱스가 읽기 인덱스보다 뒤에 있으면 쓰기 인덱스부터 씁니다.
	if (m_readIndex > m_writeIndex)
	{
		memcpy(&m_buffer[m_writeIndex], srcData, write_count);
	}
	else
	{
		const int back_space = m_size - m_writeIndex;

		// 
		if (back_space >= write_count)
		{
			memcpy(&m_buffer[m_writeIndex], srcData, write_count);
		}
		else
		{
			const int fore_space = write_count - back_space;
			memcpy(&m_buffer[m_writeIndex], &srcData[0], back_space);
			memcpy(&m_buffer[0], &srcData[back_space], fore_space);
		}
	}

	m_dataCount += write_count;
	m_writeIndex += write_count;

	if (m_writeIndex >= m_size)
		m_writeIndex -= m_size;

	return write_count;
}
