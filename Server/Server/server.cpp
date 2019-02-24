#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "../../Headers/Include.h"
#include <array>
#pragma comment(lib, "ws2_32")
using namespace std;

static const int EVT_RECV = 0;
static const int EVT_SEND = 1;
static const int EVT_MOVE = 2;
static const int EVT_PLAYER_MOVE = 3;

struct XMFLOAT3
{
	float x;
	float y;
	float z;

	XMFLOAT3() = default;

	XMFLOAT3(const XMFLOAT3&) = default;
	XMFLOAT3& operator=(const XMFLOAT3&) = default;

	XMFLOAT3(XMFLOAT3&&) = default;
	XMFLOAT3& operator=(XMFLOAT3&&) = default;
};

// ���� ���� ������ ���� ����ü�� ����
struct SOCKETINFO
{
	WSAOVERLAPPED overlapped; // overlapped ����ü
	SOCKET sock;
	char buf[BUFSIZE + 1];	// ���� ���α׷� ����
	int recvbytes;	// ��, ���� ����Ʈ ��
	int sendbytes;
	WSABUF wsabuf;	// WSABUF ����ü
};

class Client
{
public:
	SOCKET m_sock;
	SOCKETINFO m_sockinfo;
	XMFLOAT3 m_pos;
	float m_velocity = 0.f;
	bool m_connected = false;
};


array<Client, NUM_OF_PLAYER> g_clients;

// SOCKET client_sock; // accept ���� ��, �� �����尡 �����ϱ� ������ ��������.
HANDLE hReadEvent, hWriteEvent; // client_sock ��ȣ�ϱ� ����.

// �񵿱� ����� ���۰� ó�� �Լ�
DWORD WINAPI WorkerThread(LPVOID arg);
DWORD WINAPI AcceptThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK PutPlayerPos(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
// �Լ���
void SendPacket(int, void*);

// ���� ��� �Լ�
void err_quit(const char *msg);
void err_display(const char *msg);
void err_display(int errcode);

int main(int argc, char *argv[])
{
	int retval;

	// ���� �ʱ�ȭ.
	WSADATA WSAData; 
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		cout << ("Error - Can not load 'winsock.dll' file") << endl;
		return 1;
	}

	// �̺�Ʈ ��ü ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL); /// WorkerThread �����尡 g_clients ���� ���� �о����� ���� �����忡 �˸��� �뵵
	if (hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL); /// ���� �����尡 g_clients ���� ���� ���������� alertable wait ������ WorkerThread �����忡 �˸��� �뵵
	if (hWriteEvent == NULL) return 1;

	// worker thread ����
	HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
	// accept thread ����
	HANDLE hThread2 = CreateThread(NULL, 0, AcceptThread, NULL, 0, NULL);
	if (hThread == NULL || hThread2 == NULL) return 1;


	while (true) {
		// ���� �Լ��� ��������.. �̤� 
	}
	CloseHandle(hThread);
	CloseHandle(hThread2);
	return 0;
}

// ���� ������ ���� Ŭ���̾�Ʈ�� �޴� ����
DWORD WINAPI AcceptThread(LPVOID arg)
{
	int retval;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) 
		err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	int id{ -1 };

	while (1) {
		WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ���

		// accept 
		/// Ŭ���̾�Ʈ�� ������ ������
		SOCKET client_sock = accept(listen_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		/// Ŭ���̾�Ʈ id �ο�
		else {
			for (int i = 0; i < NUM_OF_PLAYER; ++i) {  
				if (false == g_clients[i].m_connected) {
					id = i;
					break;
				}
			}
			if (-1 == id) { /// ���� ���� �ʰ�
				cout << ("Excceded Users") << std::endl;
				continue;
			}

			g_clients[id].m_connected = true;
			g_clients[id].m_sock = client_sock;
		}


		// �Ѱ��ְ�, �޾Ƽ� ����.
		// p->overlapped.hEvent = (HANDLE)p->sock;

		// ���⼭ �� �ʿ䰡 ����!!
		//if (id == 1) {
		//	//  �ٸ� Ŭ���̾�Ʈ���� �� Ŭ���̾�Ʈ 1�� ���� ������ �����Ѵ�.
		//	// --------- Process --------------
		//	// 1. ���� ���� ��Ŷ ����
		//	{
		//		packet_info packetinfo;
		//		packetinfo.type = sc_put_player;
		//		packetinfo.id = id;
		//		packetinfo.size = sizeof(XMFLOAT3);
		//		/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
		//		SOCKETINFO *p = new SOCKETINFO;
		//		ZeroMemory(&p->overlapped, sizeof(p->overlapped));
		//		p->sock = client_sock;
		//		/// SetEvent(hReadEvent); //// �ʿ� ����.
		//		p->recvbytes = p->sendbytes = 0;
		//		// p->wsabuf.buf�� ������ �־���.
		//		memcpy(p->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
		//		p->wsabuf.len = BUFSIZE;

		//		DWORD sendBytes;
		//		if (WSASend(g_clients[id - 1].m_sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
		//			if (WSAGetLastError() != WSA_IO_PENDING) {
		//				printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
		//			}
		//		}
		//	}
		//	// 2. ���� ���� ��Ŷ ����
		//	{
		//		XMFLOAT3 playerpos;
		//		playerpos = 
		//		/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
		//		SOCKETINFO *p = new SOCKETINFO;
		//		ZeroMemory(&p->overlapped, sizeof(p->overlapped));
		//		p->sock = client_sock;
		//		/// SetEvent(hReadEvent); //// �ʿ� ����.
		//		p->recvbytes = p->sendbytes = 0;
		//		// p->wsabuf.buf�� ������ �־���.
		//		memcpy(p->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
		//		p->wsabuf.len = BUFSIZE;
		//		// �� Ŭ���̾�Ʈ���� �ٸ� Ŭ���̾�Ʈ�� ���� ����.
		//		if (WSASend(p->sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
		//			if (WSAGetLastError() != WSA_IO_PENDING) {
		//				printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
		//			}
		//		}
		//	}

		//}
		

		SetEvent(hWriteEvent);
	}

	// ���� ����
	WSACleanup();
	return 0;
}

// �񵿱� ����� ���� 
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;

	while (1) {
		while (1) {
			DWORD result = WaitForSingleObjectEx(hWriteEvent, INFINITE, TRUE); // ���⼭���� alertable wait����.
			// alertable wait ���¸� �����
			if (result == WAIT_OBJECT_0) break;  // �� Ŭ���̾�Ʈ�� ������ ��� - ���� ����� ��
			if (result != WAIT_IO_COMPLETION) return 1; // �񵿱� ����� �۾��� �̿� ���� �Ϸ� ��ƾ ȣ���� ���� ��� - �ٽ� alertable wait ���¿� ����
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		int id{ -1 };
		for (int i = 0; i < NUM_OF_PLAYER; ++i) {
			if (g_clients[i].m_connected == true)
				id = i; // �űԷ� ������ Ŭ���̾�Ʈ�� id�� �˾Ƴ��� ��
		}
		if (id == -1) return 1;

		getpeername(g_clients[id].m_sock, (SOCKADDR *)&clientaddr, &addrlen);
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// ���� ���� ����ü �Ҵ�� �ʱ�ȭ
		SOCKETINFO *ptr = new SOCKETINFO;
		if (ptr == NULL) {
			cout << ("[����] �޸𸮰� �����մϴ�!") << endl;
			return 1;
		}

		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = g_clients[id].m_sock;
		SetEvent(hReadEvent); // client_sock ���� ���� �о�� ��� hReadEvent�� ��ȣ ���·�
		ptr->recvbytes = ptr->sendbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		// �񵿱� ����� ����
		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes,
			&flags, &ptr->overlapped, CompletionRoutine);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSARecv()");
				return 1;
			}
		}
	}
	return 0;
}

// �񵿱� ����� ó�� �Լ�(����� �Ϸ� ��ƾ)
void CALLBACK CompletionRoutine(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	struct SOCKETINFO *socketInfo;
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	// recv ���� ��, �� ��° ��������, ����� ���� �˾ƾ�.. overlapped ����ü�� ���� �˾ƾ� �Ѵ�.
	socketInfo = (struct SOCKETINFO *)overlapped; // ������ �� ���� overlapped ����ü�� Ÿ�� ĳ����. SOCKETINFO�� ����ü�� ù ��� ������ overlapped�̱� ������,, �� �ļ���,, callback �Լ��� ���ڷ� ������ �� �߿� ������ �����Ƿ�.


	if (dataBytes == 0) // recv�� 0 ����Ʈ�� �о��� �� ? -> ���ӵ� Ŭ���̾�Ʈ���� ���� ������ �� �� (CloseSocket()�� ȣ�� �� ��.)
	{
		// Ŭ�󿡼� �����µ� ���� ��� ���� �ʿ� X
		closesocket(socketInfo->sock);
		free(socketInfo);
		return;
	}

	// ���� �������� ���� �����Ͱ� 0�� ũ����, ó�� ���� ����. �� ���� callback�� ���.
	if (socketInfo->recvbytes == 0)
	{
		// WSARecv(���� ��⿡ ����)�� �ݹ��� ���
		socketInfo->recvbytes = dataBytes; /// �̸�ŭ �޾����ϱ� �̸�ŭ ���� ������
		socketInfo->sendbytes = 0; /// �ϳ��� �� ������ ���� ������
		socketInfo->wsabuf.buf = socketInfo->buf;
		socketInfo->wsabuf.len = socketInfo->recvbytes; /// ������ ���� ������ ��ŭ�� ������ �Ѵ�.
		  
		cout << "TRACE - Receive message : " << socketInfo->buf << " ( " << dataBytes << " bytes) " << endl;

		// �����͸� �ް� �� ��.
		// 1. �޾� �� ��Ŷ�� ������ �˾Ƴ��� �Ѵ�. (���� ����)
		packet_info packetinfo;
		{
			ZeroMemory(&packetinfo, sizeof(packetinfo));
			memcpy(&packetinfo, socketInfo->buf, sizeof(packetinfo));
		}
		// 2. ��Ŷ Ÿ���� ���� �˸´� ó���� �� �ش�.
		{
			switch (packetinfo.type) {
			case cs_put_player:
			{
				if(packetinfo.id == 0){
					// Ŭ�� 0 : connect �Ϸ������ϱ� �� ���Դٴ� ��Ŷ�� ������
					// ���� : �׷� g_clients[0].m_pos �� �� ��ġ ������ �����Ұ�!! 
					{
						XMFLOAT3 pos;
						memcpy(&pos, socketInfo->buf + sizeof(packetinfo), sizeof(pos));
						g_clients[0].m_pos = pos;
					}
					
				}
				else {
					// Ŭ�� 1 : connect �Ϸ������ϱ� �� ���Դٴ� ��Ŷ�� ������
					// ���� : �׷� g_clients[1].m_pos �� �� ��ġ ������ �����Ұ�!! 
					{
						XMFLOAT3 pos;
						memcpy(&pos, socketInfo->buf + sizeof(packetinfo), sizeof(pos));
						g_clients[1].m_pos = pos;
					}
					// ���� : Ŭ�� 0���� ���� ���� ���� ������!
					{
						// 1. ���� ����
						packet_info packetinfo;
						packetinfo.type = sc_put_player;
						packetinfo.id = 1;
						packetinfo.size = sizeof(XMFLOAT3);
						/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
						SOCKETINFO *p = new SOCKETINFO;
						ZeroMemory(&p->overlapped, sizeof(p->overlapped));
						p->sock = g_clients[1].m_sock;
						p->recvbytes = p->sendbytes = 0;
						memcpy(p->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
						p->wsabuf.len = BUFSIZE;

						DWORD sendBytes;
						if (WSASend(g_clients[0].m_sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
						// 2. ���� ����
						XMFLOAT3 pos = g_clients[1].m_pos;
						/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
						ZeroMemory(&p, sizeof(p));
						ZeroMemory(&(p->overlapped), sizeof(p->overlapped));
						p->sock = g_clients[1].m_sock;
						p->recvbytes = p->sendbytes = 0;
						memcpy(p->wsabuf.buf, &(pos), sizeof(pos));
						p->wsabuf.len = BUFSIZE;
						// �� Ŭ���̾�Ʈ���� �ٸ� Ŭ���̾�Ʈ�� ���� ����.
						if (WSASend(g_clients[0].m_sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
					}
					// ���� : ������ Ŭ�� 0 ���� ������!
					{
						// 1. ���� ����
						packet_info packetinfo;
						packetinfo.type = sc_put_player;
						packetinfo.id = 0; // 0�� ������ ������ �Ŵϲ�,,
						packetinfo.size = sizeof(XMFLOAT3);
						/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
						SOCKETINFO *p = new SOCKETINFO;
						ZeroMemory(&p->overlapped, sizeof(p->overlapped));
						p->sock = g_clients[0].m_sock;
						p->recvbytes = p->sendbytes = 0;
						memcpy(p->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
						p->wsabuf.len = BUFSIZE;

						DWORD sendBytes;
						if (WSASend(g_clients[1].m_sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
						// 2. ���� ����
						XMFLOAT3 pos = g_clients[0].m_pos;
						/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
						ZeroMemory(&p, sizeof(p));
						ZeroMemory(&p->overlapped, sizeof(p->overlapped));
						p->sock = g_clients[0].m_sock;
						p->recvbytes = p->sendbytes = 0;
						memcpy(p->wsabuf.buf, &(pos), sizeof(pos));
						p->wsabuf.len = BUFSIZE;
						// �� Ŭ���̾�Ʈ���� �ٸ� Ŭ���̾�Ʈ�� ���� ����.
						if (WSASend(g_clients[1].m_sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
					}
				}
			}
			case cs_move:
			{
				// �ش� Ŭ���̾�Ʈ�� �̵� �Ϸ�� ��ǥ�� �޾ƿ´�.
				XMFLOAT3 pos;
				char buf[BUFSIZE];
				memcpy(buf, socketInfo->buf + sizeof(packetinfo), sizeof(buf));
				// �ϴ� ���� ���°ź��� �ϰ�!!
			}
			break;
			case cs_move_left:
			{
				//g_clients[packetinfo.id].m_x -= g_clients[packetinfo.id].m_velocity;
			}
			break;
			case cs_move_top:
			{
				// g_clients[packetinfo.id].m_y -= g_clients[packetinfo.id].m_velocity;
			}
			break;
			case cs_move_right:
			{

			}
			break;
			case cs_move_bottom:
			{

			}
			break;
			}
		}
		


		// ------------------------------------------------
		// ������ʹ� �����ص� �� �� ����. �ϴ� �ּ� ó�� �Ѵ�.

		//// send, recv �ϱ� ���� ������ 0���� �ʱ�ȭ �� ���.
		//memset(&(socketInfo->overlapped), 0x00, sizeof(WSAOVERLAPPED)); // overlapped IO�� �����ϰ�, recv�� ����Ǿ����Ƿ� �� recv�� �Ϸ��� overlapped ����ü�� 0���� �ʱ�ȭ�Ѵ�. �̸� ��.

		//// Ŭ���̾�Ʈ���� �� ���� �״�� ���� �ϴ� ��.
		//if (WSASend(socketInfo->sock, &(socketInfo->wsabuf), 1, &sendBytes, 0, &(socketInfo->overlapped), CompletionRoutine) == SOCKET_ERROR)
		//{
		//	if (WSAGetLastError() != WSA_IO_PENDING)
		//	{
		//		printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
		//	}
		//}
	}
	else // send callback
	{
		// WSASend(���信 ����)�� �ݹ��� ���
		socketInfo->sendbytes += dataBytes;
		socketInfo->recvbytes = 0;
		socketInfo->wsabuf.len = BUFSIZE;
		socketInfo->wsabuf.buf = socketInfo->buf;

		cout << "TRACE - Send message : " << socketInfo->buf << "( " << dataBytes << "bytes)" << endl;

		// ------------------------------------------------
		// ������ʹ� �����ص� �� �� ����. �ϴ� �ּ� ó�� �Ѵ�.
		//memset(&(socketInfo->overlapped), 0x00, sizeof(WSAOVERLAPPED)); // overlapped IO�� �����ϰ�, recv�� ����Ǿ����Ƿ� �� recv�� �Ϸ��� overlapped ����ü�� 0���� �ʱ�ȭ�Ѵ�. �̸� ��.

		//if (WSARecv(socketInfo->sock, &socketInfo->wsabuf, 1, &receiveBytes, &flags, &(socketInfo->overlapped), CompletionRoutine) == SOCKET_ERROR)
		//{
		//	if (WSAGetLastError() != WSA_IO_PENDING)
		//	{
		//		printf("Error - Fail WSARecv(error_code : %d)\n", WSAGetLastError());
		//	}
		//}
	}
}

void CALLBACK PutPlayerPos(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	struct SOCKETINFO *socketInfo;
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	socketInfo = (struct SOCKETINFO *)overlapped; 


	if (dataBytes == 0) { // ���ӵ� Ŭ���̾�Ʈ���� ���� ������ �� �� 
		closesocket(socketInfo->sock);
		free(socketInfo);
		return;
	}

	// ���� callback�� ���.
	if (socketInfo->recvbytes == 0) {
	}
	else // send callback
	{
		// WSASend(���信 ����)�� �ݹ��� ���
		socketInfo->sendbytes += dataBytes;
		socketInfo->recvbytes = 0;
		socketInfo->wsabuf.len = BUFSIZE;
		socketInfo->wsabuf.buf = socketInfo->buf;

		cout << "TRACE - Send message : " << socketInfo->buf << "( " << dataBytes << "bytes)" << endl;

		// ------------------------------------------------
		// ������ʹ� �����ص� �� �� ����. �ϴ� �ּ� ó�� �Ѵ�.
		//memset(&(socketInfo->overlapped), 0x00, sizeof(WSAOVERLAPPED)); // overlapped IO�� �����ϰ�, recv�� ����Ǿ����Ƿ� �� recv�� �Ϸ��� overlapped ����ü�� 0���� �ʱ�ȭ�Ѵ�. �̸� ��.

		//if (WSARecv(socketInfo->sock, &socketInfo->wsabuf, 1, &receiveBytes, &flags, &(socketInfo->overlapped), CompletionRoutine) == SOCKET_ERROR)
		//{
		//	if (WSAGetLastError() != WSA_IO_PENDING)
		//	{
		//		printf("Error - Fail WSARecv(error_code : %d)\n", WSAGetLastError());
		//	}
		//}
	}
}

void SendPacket(int id, void *ptr)
{
	//unsigned char *packet = reinterpret_cast<unsigned char *>(ptr);
	//EXOVER *s_over = new EXOVER;
	//s_over->event_type = EVT_SEND;
	//memcpy(s_over->m_iobuf, packet, packet[0]);
	//s_over->m_wsabuf.buf = s_over->m_iobuf;
	//s_over->

	//int retval;
	//DWORD sendbytes;
	//retval = WSASend(g_clients[id].m_sockinfo.sock, (g_clients[id].m_sockinfo.wsabuf), 1, &sendbytes, 0, &(g_clients[id].m_sockinfo.overlapped), CompletionRoutine);
	//if (retval == SOCKET_ERROR) {
	//	if (WSAGetLastError() != WSA_IO_PENDING) {
	//		err_display("WSASend()");
	//		return;
	//	}
	//}
}
// ���� �Լ� ���� ��� �� ����
void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// ���� �Լ� ���� ���
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[����] %s", (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}