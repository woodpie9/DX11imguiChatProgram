#pragma once
#include <assert.h>
#include "WinSocket.h"
#include "StreamQueue.h"

// 20230328 �ڵ� ��Ÿ�� ����
// 20230314 woodpie9 �ڵ� ���� 
// WinSocket�� ��� �޴� TCP ������ �����ϴ� Ŭ����


WOODNET_BEGIN

class TCPSocket : public WinSocket
{
	struct TCPBuffer
	{
		short count;		// �������� �� ����
		short done;			// ���� �������� ��
		short buf_size;
		char* p_buffer;
	};

public:
	TCPSocket(const TCPSocket&) = delete;
	TCPSocket& operator=(const TCPSocket&) = delete;

	TCPSocket();
	~TCPSocket();

	// Ŭ���̾�Ʈ�� ������ �����ϱ� ���� IP�ּҿ� ��Ʈ��ȣ ����
	bool Connect(SOCKADDR_IN& remote_addr) const;
	// ������ ������ ������ ��� ���Ͽ� ������ ��ġ�մϴ�.
	bool Listen();

	// ����� ���Ͽ� �����͸� �����ϴ�.
	int	Send(char* p_out_buf, int len, int& error, LPWSAOVERLAPPED p_ov = NULL, LPWSAOVERLAPPED_COMPLETION_ROUTINE lp_completion_routine = NULL) const;
	// ����� ���� �Ǵ� ���ε��� ���� ���� ���Ͽ��� �����͸� �޽��ϴ�.
	int	Recv(char* p_in_buf, int len, int& error, LPWSAOVERLAPPED p_ov = NULL, LPWSAOVERLAPPED_COMPLETION_ROUTINE lp_completion_routine = NULL);


	bool SendUpdate();		// �����͸� 
	bool RecvUpdate();

	bool PostPacket(char* buffer, int size) const;		// �����͸� ť�� ���ϴ�.
	bool PeekPacket(char* buffer, int size) const;		// �����͸� ť���� ������ �ʰ� �н��ϴ�.
	bool ReadPacket(char* buffer, int size) const;		// �����͸� ť���� �н��ϴ�.

	void Reset();


	void SetNetId(NetworkObjectID id) { net_id_ = id; };
	NetworkObjectID GetNetId() { return net_id_; };


protected:
	NetworkObjectID net_id_;

private:
	TCPBuffer send_buf_;		// �۽Ź���. �۽��� �����͸� �����ϴ� ����
	TCPBuffer recv_buf_;		// ���Ź���. ������ �����͸� �����ϴ� ����

	StreamQueue* send_q_;		// �۽Ź���. ���α׷����� �۽��� �����͸� �����ϴ� ��Ʈ�� ť
	StreamQueue* recv_q_;		// ���Ź���. ���α׷��� ������ �����͸� �����ϴ� ��Ʈ�� ť
};


WOODNET_END