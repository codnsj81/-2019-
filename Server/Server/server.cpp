#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "../../Headers/Include.h"
#include <array>
using namespace std;
#pragma comment(lib, "ws2_32")


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
	char buf[BUFSIZE + 1];	// ���� ���α׷� ����. ���� ���۰� �� �޽��� ����.
	int recvbytes;	// ��, ���� ����Ʈ ��
	int sendbytes;
	WSABUF wsabuf;	// WSABUF ����ü. ���� ��ü�� �ƴ�, ������ ������
};

class Client
{
public:
	SOCKET m_sock;
	SOCKETINFO m_sockinfo;
	XMFLOAT3 m_pos;
	player_type m_type;
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

	// ����� ������ �����Ѵ�.
	SOCKET listen_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listen_sock == INVALID_SOCKET) err_quit("[����] Invalid socket");

	// ���� ���� ��ü�� �����Ѵ�.
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = PF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	// ������ �����Ѵ�.
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		closesocket(listen_sock);
		WSACleanup();
		err_quit("[����] bind()");
	}

	// ���� ��⿭�� �����Ѵ�.
	retval = listen(listen_sock, 2);
	if (retval == SOCKET_ERROR) {
		closesocket(listen_sock);
		WSACleanup();
		err_quit("[����] listen()");
	}

	//// accept thread ����
	//HANDLE hThread = CreateThread(NULL, 0, AcceptThread, NULL, 0, NULL);
	//if (hThread == NULL) return 1;


	SOCKADDR_IN client_addr;
	int addr_length = sizeof(SOCKADDR_IN);
	memset(&client_addr, 0, addr_length);
	SOCKET client_sock;
	SOCKETINFO* socket_info;
	DWORD recv_bytes;
	DWORD flags;

	int id{ -1 };
	while (1) {
		// accept -> Ŭ���̾�Ʈ�� ���� -> Ŭ���̾�Ʈ�� �ּ� ����ü ������ ���� -> �ּ� ���� ����.
		client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &addr_length);
		if (client_sock == INVALID_SOCKET) {
			err_display("[����] accept()");
			return 1;
		}
		
		// Ŭ���̾�Ʈ ������ ����� �´�.
		socket_info = new SOCKETINFO;
		memset((void*)socket_info, 0x00, sizeof(SOCKETINFO));
		socket_info->sock = client_sock;
		socket_info->wsabuf.len = MAX_BUFSIZE;
		socket_info->wsabuf.buf = socket_info->buf;
		flags = 0;

		// �Ѱ��ְ�, �޾Ƽ� ����.
		// socket_info->overlapped.hEvent = (HANDLE)socket_info->sock;

		// ��ø ���� ���� -> �Ϸ�� ����� �Լ� �Ѱ���
		// Recv �ϰ� �ٷ� ������ -> overlapped IO��. ���� ���ۿ� �����Ͱ� ������ �� ���� ��.
		if (WSARecv(socket_info->sock, &socket_info->wsabuf, 1, &recv_bytes, &flags, &(socket_info->overlapped), CompletionRoutine)) {
			// overlapped IO�� pending ������ ���;� ��.
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_quit("[����] IO Pending Fail");
			}
		}
		// ������ Ŭ���̾�Ʈ�� ������ ����.
		cout << "[�˸�] [" << id << "] ��° Ŭ���̾�Ʈ ���� : IP �ּ� = " << inet_ntoa(client_addr.sin_addr) << ", ��Ʈ ��ȣ = " << ntohs(client_addr.sin_port) << endl;


		// Ŭ���̾�Ʈ�� id�� �ο��Ѵ�.
		for (int i = 0; i < NUM_OF_PLAYER; ++i) {
			if (false == g_clients[i].m_connected) {
				id = i;
				break;
			}
		}
		if (-1 == id) { /// ���� ���� �ʰ�
			cout << ("Excceded Users") << endl;
			continue;
		}

		// g_clients[�ش�id] �迭 ���ҿ� ������ �����Ѵ�.
		g_clients[id].m_connected = true;
		g_clients[id].m_sock = client_sock;
		memcpy(&(g_clients[id].m_sockinfo), socket_info, sizeof(SOCKETINFO));

	}
	closesocket(listen_sock);
	WSACleanup();
	return 0;

}

// ���� ������ ���� Ŭ���̾�Ʈ�� �޴� ����
DWORD WINAPI AcceptThread(LPVOID arg)
{


	return 0;
}

// �񵿱� ����� ���� 
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;

	while (1) {
		while (1) {
			//DWORD result = WaitForSingleObjectEx(hWriteEvent, INFINITE, TRUE); // ���⼭���� alertable wait����.
			// alertable wait ���¸� �����
			//if (result == WAIT_OBJECT_0) break;  // �� Ŭ���̾�Ʈ�� ������ ��� - ���� ����� ��
			//if (result != WAIT_IO_COMPLETION) return 1; // �񵿱� ����� �۾��� �̿� ���� �Ϸ� ��ƾ ȣ���� ���� ��� - �ٽ� alertable wait ���¿� ����
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
		//SetEvent(hReadEvent); // client_sock ���� ���� �о�� ��� hReadEvent�� ��ȣ ���·�
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


	// ���ӵ� Ŭ���̾�Ʈ���� ���� ������ ������ ��.
	if (dataBytes == 0)
	{
		// recv�� 0 ����Ʈ�� �о��� �� ? -> ���ӵ� Ŭ���̾�Ʈ���� ���� ������ �� �� (CloseSocket()�� ȣ�� �� ��.)
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

		// �����͸� �ް� �� ��.
		// 1. �޾� �� ��Ŷ�� ������ �˾Ƴ��� �Ѵ�. (���� ����)
		packet_info packetinfo;
		{
			ZeroMemory(&packetinfo, sizeof(packetinfo));
			memcpy(&packetinfo, socketInfo->buf, sizeof(packetinfo));
		}
		cout << "Receive packet : " << packetinfo.type << " ( " << dataBytes << " bytes) " << endl;
		// 2. ��Ŷ Ÿ���� ���� �˸´� ó���� �� �ش�.
		{
			switch (packetinfo.type) {
			case cs_put_player:
			{
				if (false == g_clients[1].m_connected) {
					// Ŭ�� 0 : connect �Ϸ������ϱ� �� ���Դٴ� ��ȣ, ��ġ ���� ��Ŷ ������
					// ���� : g_clients[0].m_pos�� �� ��ġ ������ �����Ұ�!! 
					player_info playerinfo;
					{
						memcpy(&playerinfo, socketInfo->buf + sizeof(packetinfo), sizeof(playerinfo));
						g_clients[0].m_pos.x = playerinfo.x; g_clients[0].m_pos.y = playerinfo.y; g_clients[0].m_pos.z = playerinfo.z;
						g_clients[0].m_type = playerinfo.type;
						playerinfo.id = 0;
					}
					// ���� : ������ id�� �ο��ؼ� ���ŵ� player_info�� �˷� �ٰ�!
					{
						// 1. ���� ����
						packet_info packetinfo;
						packetinfo.type = sc_your_playerinfo;
						packetinfo.id = 0;
						packetinfo.size = sizeof(player_info);
						/*/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
						SOCKETINFO *p = new SOCKETINFO;*/
						ZeroMemory(&(socketInfo->overlapped), sizeof(socketInfo->overlapped));
						//p->sock = g_clients[0].m_sock;
						//p->recvbytes = p->sendbytes = 0;
						memcpy(socketInfo->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
						//p->wsabuf.len = BUFSIZE;
						// 2. ���� ����
						playerinfo.x = g_clients[0].m_pos.x;
						playerinfo.y = g_clients[0].m_pos.y;
						playerinfo.z = g_clients[0].m_pos.z;
						playerinfo.connected = true;
						/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
						/*p->sock = g_clients[0].m_sock;
						p->recvbytes = p->sendbytes = 0;*/
						memcpy(socketInfo->wsabuf.buf + sizeof(packetinfo), &(playerinfo), sizeof(playerinfo));
						socketInfo->wsabuf.len = BUFSIZE;
						// ����
						if (WSASend(socketInfo->sock, &(socketInfo->wsabuf), 1, &sendBytes, 0, &(socketInfo->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
					}

				}
				else if (true == g_clients[1].m_connected) {
					// Ŭ�� 1 : connect �Ϸ������ϱ� �� ���Դٴ� ��Ŷ�� ������
					// ���� : �׷� g_clients[1]�� �� ������ �����Ұ�!! 
					player_info playerinfo;
					{
						memcpy(&playerinfo, socketInfo->buf + sizeof(packetinfo), sizeof(playerinfo));
						g_clients[1].m_pos.x = playerinfo.x; g_clients[1].m_pos.y = playerinfo.y; g_clients[1].m_pos.z = playerinfo.z;
						g_clients[1].m_type = playerinfo.type;
						playerinfo.id = 1;
					}
					// ���� : ������ id�� �ο��ϰ� ���ŵ� player_info�� ������!
					{
						// 1. ���� ����
						packet_info packetinfo;
						packetinfo.type = sc_your_playerinfo;
						packetinfo.id = 1;
						packetinfo.size = sizeof(player_info);
						ZeroMemory(&(socketInfo->overlapped), sizeof(socketInfo->overlapped));
						memcpy(socketInfo->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
						// 2. ���� ����
						playerinfo.x = g_clients[1].m_pos.x;
						playerinfo.y = g_clients[1].m_pos.y;
						playerinfo.z = g_clients[1].m_pos.z;
						playerinfo.connected = true;
						memcpy(socketInfo->wsabuf.buf + sizeof(packetinfo), &(playerinfo), sizeof(playerinfo));
						socketInfo->wsabuf.len = BUFSIZE;
						// ����
						if (WSASend(socketInfo->sock, &(socketInfo->wsabuf), 1, &sendBytes, 0, &(socketInfo->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
					}
					// ���� : Ŭ�� 0���� ���� ���� ���� ������!
					{
						// 1. ���� ����
						packet_info packetinfo;
						packetinfo.type = sc_put_player;
						packetinfo.id = 1;
						packetinfo.size = sizeof(player_info);
						/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
						SOCKETINFO *p = new SOCKETINFO;
						memcpy(p, &(g_clients[0].m_sockinfo), sizeof(*p));
						ZeroMemory(&p->overlapped, sizeof(p->overlapped));
						p->recvbytes = p->sendbytes = 0;
						memcpy(p->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
						p->wsabuf.len = BUFSIZE;

						// 2. ���� ����
						memcpy(p->wsabuf.buf + sizeof(packetinfo), &(playerinfo), sizeof(playerinfo));
						p->wsabuf.len = BUFSIZE;
						// �� Ŭ���̾�Ʈ���� �ٸ� Ŭ���̾�Ʈ�� ���� ����.
						if (WSASend(p->sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
					}
					// ���� : �����״� Ŭ�� 0 �� ���� ������!
					{
						// 1. ���� ����
						packet_info packetinfo;
						packetinfo.type = sc_put_player;
						packetinfo.id = 0;
						packetinfo.size = sizeof(player_info);
						memcpy(socketInfo->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
						// 2. ���� ����
						playerinfo.x = g_clients[0].m_pos.x;
						playerinfo.y = g_clients[0].m_pos.y;
						playerinfo.z = g_clients[0].m_pos.z;
						playerinfo.type = g_clients[0].m_type;
						playerinfo.connected = true;
						memcpy(socketInfo->wsabuf.buf + sizeof(packetinfo), &(playerinfo), sizeof(playerinfo));
						socketInfo->wsabuf.len = BUFSIZE;
						ZeroMemory(&(socketInfo->overlapped), sizeof(socketInfo->overlapped));
						// ����
						if (WSASend(socketInfo->sock, &(socketInfo->wsabuf), 1, &sendBytes, 0, &(socketInfo->overlapped), PutPlayerPos) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
							}
						}
					}
				}
			}
			break;
			case cs_move:
			{
				// �ش� Ŭ���̾�Ʈ�� �̵� �Ϸ�� ��ǥ�� �޾ƿ´�.
				player_info playerinfo;
				{
					memcpy(&playerinfo, socketInfo->buf + sizeof(packetinfo), sizeof(playerinfo));
					g_clients[playerinfo.id].m_pos.x = playerinfo.x;
					g_clients[playerinfo.id].m_pos.y = playerinfo.y;
					g_clients[playerinfo.id].m_pos.z = playerinfo.z;
				}

				// 0�̸� 1����, 1�̸� 0���� sc_notify_pos ��Ŷ�� ������.
				{
					// 0. ���� Ŭ���̾�Ʈ�� id ���ϱ�.
					int recvid{ -1 };
					if (playerinfo.id == 0)
						recvid = 1;
					else
						recvid = 0;

					// 1. ���� ����
					packet_info packetinfo;
					packetinfo.type = sc_notify_pos;
					packetinfo.id = playerinfo.id;
					packetinfo.size = sizeof(player_info);
					/// SOCKETINFO �Ҵ�, �ʱ�ȭ.
					SOCKETINFO *p = new SOCKETINFO;
					memcpy(p, &(g_clients[recvid].m_sockinfo), sizeof(*p));
					ZeroMemory(&p->overlapped, sizeof(p->overlapped));
					p->recvbytes = p->sendbytes = 0;
					memcpy(p->wsabuf.buf, &(packetinfo), sizeof(packetinfo));
					p->wsabuf.len = BUFSIZE;

					// 2. ���� ����
					memcpy(p->wsabuf.buf + sizeof(packetinfo), &(playerinfo), sizeof(playerinfo));
					p->wsabuf.len = BUFSIZE;
					// �� Ŭ���̾�Ʈ���� �ٸ� Ŭ���̾�Ʈ�� ���� ����.
					if (WSASend(p->sock, &(p->wsabuf), 1, &sendBytes, 0, &(p->overlapped), PutPlayerPos) == SOCKET_ERROR) {
						if (WSAGetLastError() != WSA_IO_PENDING) {
							printf("Error - F ail WSASend(error_code : %d)\n", WSAGetLastError());
						}
					}

				}
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