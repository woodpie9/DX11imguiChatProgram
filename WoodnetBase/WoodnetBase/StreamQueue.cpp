// ReSharper disable CppClangTidyClangDiagnosticInvalidUtf8
#include "StreamQueue.h"

#include <algorithm>
#include <cstring>

void woodnet::StreamQueue::clear()
{
	// 변수들과 버퍼를 0으로 반환합니다.
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
	if (peekLen > count())
		return false;

	// 쓰기 인덱스가 더 크면 처음부터 읽기 인덱스까지 복사합니다.
	if (m_readIndex < m_writeIndex)
	{
		memcpy(peekBuf, &m_buffer[m_readIndex], peekLen);
	}
	else
	{
		// 읽기 인덱스로부터 큐의 끝까지의 크기를 구합니다.
		const int back_part = m_size - m_readIndex;

		if (peekLen <= back_part)
		{
			memcpy(peekBuf, &m_buffer[m_readIndex], peekLen);
		}
		else
		{
			// 남은 peek 길이를 구합니다.
			const int fore_part = peekLen - back_part;

			memcpy(&peekBuf[0], &m_buffer[m_readIndex], back_part);
			memcpy(&peekBuf[back_part], &m_buffer[0], fore_part);
		}
	}

	return true;
}

int woodnet::StreamQueue::read(char* desBuf, int bufLen)
{
	if (is_empty()) return 0;

	// 버퍼 내에 있는 데이터 수 or 원하는 데이터 의 수 중에 작은 수만큼 읽는다.
	const int read_count = std::min<short>(m_dataCount, bufLen);

	// 읽기 인덱스가 쓰기 인덱스보다 앞에 있으면 처음부터 읽기 인덱스까지 복사
	if (m_readIndex < m_writeIndex)
	{
		memcpy(desBuf, &m_buffer[m_readIndex], read_count);
	}
	else
	{
		// 읽기 인덱스로부터 큐의 끝까지의 크기를 구합니다.
		int back_part = m_size - m_readIndex;

		if (read_count <= back_part)
		{

			memcpy(desBuf, &m_buffer[m_readIndex], read_count);
		}
		else
		{
			int fore_part = read_count - back_part;
			memcpy(&desBuf[0], &m_buffer[m_readIndex], back_part);
			memcpy(&desBuf[back_part], &m_buffer[0], fore_part);
		}
	}

	m_dataCount -= read_count;
	m_readIndex += read_count;

	if (m_readIndex >= m_size)
		m_readIndex -= m_size;

	return read_count;
}

int woodnet::StreamQueue::write(const char* srcData, int bytesData)
{
	if (is_full()) return 0;

	const int left = remain();
	const int write_count = std::min<short>(left, bytesData);

	if (m_readIndex > m_writeIndex)
	{
		memcpy(&m_buffer[m_writeIndex], srcData, write_count);
	}
	else
	{
		const int back_space = m_size - m_writeIndex;

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
