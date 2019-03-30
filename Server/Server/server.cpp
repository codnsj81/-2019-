#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "../../Headers/Include.h"
#include <map>
using namespace std;
#pragma comment(lib, "ws2_32")


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
	WSABUF wsabuf;	// WSABUF ����ü. ���� ��ü�� �ƴ�, ������ ������
	SOCKET sock;
	char buf[BUFSIZE + 1];	// ���� ���α׷� ����. ���� ���۰� �� �޽��� ����.
	int recvbytes;	// ��, ���� ����Ʈ ��
	int sendbytes;
	player_info playerinfo;
};

bool g_connected[2];
map<SOCKET, SOCKETINFO> clients;

// SOCKET client_sock; // accept ���� ��, �� �����尡 �����ϱ� ������ ��������.
HANDLE hReadEvent, hWriteEvent; // client_sock ��ȣ�ϱ� ����.

// �񵿱� ����� ���۰� ó�� �Լ�
void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_nonrecv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_scputplayer_1_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_yesrecv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);

// �Լ���
void show_allplayer();

// ���� ��� �Լ�
void err_quit(const char *msg);
void err_display(const char *msg);

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
	if (listen_sock == INVALID_SOCKET)
		err_quit("[����] Invalid socket");

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
	retval = listen(listen_sock, NUM_OF_PLAYER);
	if (retval == SOCKET_ERROR) {
		closesocket(listen_sock);
		WSACleanup();
		err_quit("[����] listen()");
	}


	SOCKADDR_IN client_addr;
	int addr_length = sizeof(SOCKADDR_IN);
	memset(&client_addr, 0, addr_length);
	SOCKET client_sock;
	DWORD flags;

	int id{ -1 };
	while (1) {
		// accept 
		client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &addr_length);
		if (client_sock == INVALID_SOCKET) {
			err_display("[����] accept()");
			return 1;
		}
		// ������ Ŭ���̾�Ʈ ���� ���
		cout << "[Ŭ���̾�Ʈ ����] IP �ּ� (" << inet_ntoa(client_addr.sin_addr) <<
			"), ��Ʈ ��ȣ (" << ntohs(client_addr.sin_port) << ")" << endl;
		//WSARecv �غ�.
		clients[client_sock] = SOCKETINFO{};
		memset(&clients[client_sock], 0x00, sizeof(struct SOCKETINFO));
		clients[client_sock].sock = client_sock;
		clients[client_sock].wsabuf.len = BUFSIZE;
		clients[client_sock].wsabuf.buf = clients[client_sock].buf;
		flags = 0;
		// ��ø ������ ����.
		clients[client_sock].overlapped.hEvent = (HANDLE)clients[client_sock].sock;

		if (WSARecv(clients[client_sock].sock, &clients[client_sock].wsabuf,
			1, NULL, &flags, &(clients[client_sock].overlapped), recv_callback)) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("IO pending Failure");
				return 0;
			}
		}
		else {
			cout << ("Non overlapped recv return") << endl;
		}

		cout << "���� ���� �� Ŭ���̾�Ʈ �� : " << clients.size() << endl;

	}

	// closesocket()
	closesocket(listen_sock);
	// ���� ����
	WSACleanup();

}


// �񵿱� ����� ó�� �Լ�(����� �Ϸ� ��ƾ)
void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	// �������忡�� ������ �о�´�.
	SOCKET client_sock = reinterpret_cast<int>(overlapped->hEvent);

	// �ٽ� recv �����ؾ� ��.
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;

	if (dataBytes == 0) {
		// Ŭ���̾�Ʈ ���� ����.
		closesocket(clients[client_sock].sock);
		clients.erase(client_sock);
		return;
	}

	packet_info packetinfo;
	player_info playerinfo;

	// ���� ����.
	memcpy(&packetinfo, clients[client_sock].buf, sizeof(packetinfo));
	// ���� ����.
	switch (packetinfo.type) {
	case cs_put_player:
	{
		// Ŭ���̾�Ʈ�� �ʱ⿡ �����Ͽ� send �� playerinfo�� �޾ƿ´�.
		memcpy(&playerinfo, clients[client_sock].buf + sizeof(packetinfo),
			sizeof(playerinfo));
		// id�� �ο��ϰ�, clients[client_sock]�� playerinfo�� �����Ѵ�.
		{
			int id = -1;
			for (int i = 0; i < NUM_OF_PLAYER; ++i) {
				if (false == g_connected[i]) {
					g_connected[i] = true;
					id = i;
					break;
				}
			}
			playerinfo.id = id; // ������ id�� playerinfo�� ä���.
			clients[client_sock].playerinfo = playerinfo; // clients[client_sock]�� playerinfo�� �ִ´�.
		}
		// �ش� Ŭ���̾�Ʈ���� id�� ������ �ڽ��� playerinfo�� send�Ѵ�.
		{
			// ���� ���� �غ�. ���� ���� + ���� ���� ������ �̾� ����.
			char buf[BUFSIZE];
			memset(&(packetinfo), 0x00, sizeof(packet_info));
			// packetinfo ä����.
			packetinfo.id = playerinfo.id;
			packetinfo.size = sizeof(packet_info);
			packetinfo.type = sc_notify_yourinfo;
			// buf�� packetinfo + playerinfo �����ϰ�.
			memcpy(buf, &packetinfo, sizeof(packet_info));
			memcpy(buf + sizeof(packet_info), &playerinfo, sizeof(player_info));
			// buf�� messageBuffer�� �����ϰ�.
			memcpy(clients[client_sock].buf, buf, sizeof(clients[client_sock].buf));
			// ���̴� ����+���� ũ��.
			clients[client_sock].wsabuf.len = sizeof(packetinfo) + sizeof(playerinfo);
			// �������� ����ü ����. �ʱ�ȭ.
			memset(&(clients[client_sock].overlapped), 0x00, sizeof(WSAOVERLAPPED));
			clients[client_sock].overlapped.hEvent = (HANDLE)client_sock;
			if (WSASend(client_sock, &(clients[client_sock].wsabuf), 1, &dataBytes, 0, &(clients[client_sock].overlapped),
				send_scputplayer_1_callback) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
				}
			}
		}

		// �ٸ� Ŭ���̾�Ʈ�� �ִٸ�, �� Ŭ���̾�Ʈ�� ���� ������ + playerinfo�� send �Ѵ�.
		{
			// ���� ���� �غ�. ���� ���� + ���� ���� ������ �̾� ����.
			char buf[BUFSIZE];
			memset(&(packetinfo), 0x00, sizeof(packet_info));
			// packetinfo ä����.
			packetinfo.id = playerinfo.id;
			packetinfo.size = sizeof(packet_info);
			packetinfo.type = sc_put_player;
			// buf�� packetinfo + playerinfo �����ϰ�.
			memcpy(buf, &packetinfo, sizeof(packet_info));
			memcpy(buf + sizeof(packet_info), &playerinfo, sizeof(player_info));
			// map �����̳ʸ� ���� ���� ���� Ŭ���̾�Ʈ���� ��� ������.
			for (map<SOCKET, SOCKETINFO>::iterator i_b = clients.begin(); i_b != clients.end(); i_b++)
			{
				SOCKET other_sock = i_b->second.sock;
				if (other_sock == client_sock) // �����Դ� �ٽ� �� ������ ��.
					continue;

				// -------------------------------------------------
				// send, recv �������� �𸣴� other_sock�� overlappd ����ü�� wsabuf�� �ǵ������ �� �ȴ�.
				// -> ���� �Ҵ��Ѵ�.

				//WSASend �غ�.
				// ���1)
				WSAOVERLAPPED* pOver = new WSAOVERLAPPED;
				pOver->hEvent = (HANDLE)other_sock;
				WSABUF* pWsabuf = new WSABUF;
				char* pBuf = new char[BUFSIZE];
				pWsabuf->len = BUFSIZE;
				pWsabuf->buf = pBuf;
				memcpy(pBuf, buf, BUFSIZE);
				DWORD recvBytes = 0;
				if (WSASend(other_sock, pWsabuf, 1, &recvBytes, 0, pOver,
					send_callback) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
					}
				}
	
				////���2)
				//player_info other_playerinfo = clients[other_sock].playerinfo;
				//clients[other_sock] = SOCKETINFO{};
				//memset(&clients[other_sock], 0x00, sizeof(struct SOCKETINFO));
				//clients[other_sock].playerinfo = other_playerinfo;
				//clients[other_sock].sock = other_sock;
				//clients[other_sock].wsabuf.len = BUFSIZE;
				//clients[other_sock].wsabuf.buf = clients[other_sock].buf;
				//int other_flags = 0;
				//clients[other_sock].overlapped.hEvent = (HANDLE)clients[other_sock].sock;
				// memcpy(clients[other_sock].buf, buf, sizeof(clients[other_sock].buf));
				//// �������� ����ü ����. �ʱ�ȭ. 
				//memset(&(clients[other_sock].overlapped), 0x00, sizeof(WSAOVERLAPPED));
				//clients[other_sock].overlapped.hEvent = (HANDLE)(i_b->second.sock);
				//if (WSASend(other_sock, pWsabuf, 1, &dataBytes, 0, pOver,
				//	send_callback) == SOCKET_ERROR)
				//{
				//	if (WSAGetLastError() != WSA_IO_PENDING)
				//	{
				//		printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
				//	}
				//}
			}
		}
	}
	break;
	case cs_move:
	{
		// ������ Ŭ���̾�Ʈ�� ������ �޾ƿ´�.
		{
			char buf[BUFSIZE];
			memcpy(&playerinfo, clients[client_sock].buf + sizeof(packetinfo), sizeof(player_info));
			clients[client_sock].playerinfo = playerinfo;
		}
		// �ٸ� ���� ���� Ŭ���̾�Ʈ���� ������ Ŭ���̾�Ʈ�� ������ ������.
		{
			char buf[BUFSIZE];
			playerinfo.id = packetinfo.id; // ������ Ŭ���̾�Ʈ�� ���̵�.
			// �ش� Ŭ���̾�Ʈ���� ���� ���ŵ� ���� ����. (����)
			memset(&packetinfo, 0x00, sizeof(packetinfo));
			packetinfo.id = playerinfo.id;
			packetinfo.size = sizeof(playerinfo);
			packetinfo.type = sc_notify_playerinfo;
			memcpy(buf, &packetinfo, sizeof(packetinfo));
			// (����)
			memcpy(buf + sizeof(packetinfo), &(clients[client_sock].playerinfo), sizeof(player_info));

			if (clients.size() > 1) {

				for (map<SOCKET, SOCKETINFO>::iterator i_b = clients.begin(); i_b != clients.end(); i_b++)
				{
					SOCKET other_sock = i_b->second.sock;
					if (other_sock == client_sock) // �����Դ� �ٽ� �� ������ ��.
						continue;
					//WSASend �غ�.
					// ���1)
					WSAOVERLAPPED* pOver = new WSAOVERLAPPED;
					ZeroMemory(pOver, sizeof(WSAOVERLAPPED));
					pOver->hEvent = (HANDLE)other_sock;
					WSABUF* pWsabuf = new WSABUF;
					char* pBuf = new char[BUFSIZE];
					pWsabuf->len = BUFSIZE;
					pWsabuf->buf = pBuf;
					memcpy(pBuf, buf, BUFSIZE);
					DWORD recvBytes = 0;
					if (WSASend(other_sock, pWsabuf, 1, &recvBytes, 0, pOver,
						send_callback) == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
						}
					}
					// ���2)
					//player_info other_playerinfo = clients[other_sock].playerinfo;
					//clients[other_sock] = SOCKETINFO{};
					//memset(&clients[other_sock], 0x00, sizeof(struct SOCKETINFO));
					//clients[other_sock].playerinfo = other_playerinfo;
					//clients[other_sock].sock = other_sock;
					//clients[other_sock].wsabuf.len = BUFSIZE;
					//clients[other_sock].wsabuf.buf = clients[other_sock].buf;
					//int other_flags = 0;
					//clients[other_sock].overlapped.hEvent = (HANDLE)clients[other_sock].sock;
					//memcpy(clients[other_sock].buf, buf, sizeof(clients[other_sock].buf));
					//clients[other_sock].wsabuf.len = sizeof(packetinfo) + sizeof(playerinfo);
					//// �������� ����ü ����. �ʱ�ȭ. 
					//memset(&(clients[other_sock].overlapped), 0x00, sizeof(WSAOVERLAPPED));
					//clients[other_sock].overlapped.hEvent = (HANDLE)(i_b->second.sock);
					//if (WSASend(other_sock, pWsabuf, 1, &dataBytes, 0, pOver,
					//	send_callback) == SOCKET_ERROR)
					//{
					//	if (WSAGetLastError() != WSA_IO_PENDING)
					//	{
					//		printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
					//	}
					//}
				}
			}
			else
			{
				DWORD flags = 0;
				// WSASend(���信 ����)�� �ݹ��� ���
				clients[client_sock].wsabuf.len = BUFSIZE;
				clients[client_sock].wsabuf.buf = clients[client_sock].buf;

				memset(&(clients[client_sock].overlapped), 0x00, sizeof(WSAOVERLAPPED));
				clients[client_sock].overlapped.hEvent = (HANDLE)client_sock;


				if (WSARecv(client_sock, &clients[client_sock].wsabuf, 1,
					NULL/* ��������: �츮 null �� �ֱ�� ���ݾ�,, */, &flags, &(clients[client_sock].overlapped), recv_callback) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						printf("Error - Fail WSARecv(error_code : %d)\n", WSAGetLastError());
					}
				}
			}
		}
	}
	break;
	}
	show_allplayer();
}

void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);


	if (dataBytes == 0)
	{
		closesocket(clients[client_s].sock);
		clients.erase(client_s);
		return;
	}  // Ŭ���̾�Ʈ�� closesocket�� ���� ���

		// WSASend(���信 ����)�� �ݹ��� ���
	clients[client_s].wsabuf.len = BUFSIZE;
	clients[client_s].wsabuf.buf = clients[client_s].buf;

	memset(&(clients[client_s].overlapped), 0x00, sizeof(WSAOVERLAPPED));
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;


	if (WSARecv(client_s, &clients[client_s].wsabuf, 1,
		NULL/* ��������: �츮 null �� �ֱ�� ���ݾ�,, */, &flags, &(clients[client_s].overlapped), recv_callback) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			printf("Error - Fail WSARecv(error_code : %d)\n", WSAGetLastError());
		}
	}

}


void CALLBACK send_scputplayer_1_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	if (dataBytes == 0)
	{
		closesocket(clients[client_s].sock);
		clients.erase(client_s);
		return;
	}  // Ŭ���̾�Ʈ�� closesocket�� ���� ���

	// 3�� �ܰ� ���� or WSARecv ����
	// �ش� Ŭ���̾�Ʈ���� �ٸ� Ŭ���̾�Ʈ�� playerinfo�� send �Ѵ�.
	int count = clients.size();
	if (clients.size() > 1) {
		for (map<SOCKET, SOCKETINFO>::iterator i_b = clients.begin(); i_b != clients.end(); i_b++) {
			if (clients[client_s].playerinfo.id == i_b->second.playerinfo.id)
				continue;
			char buf[BUFSIZE];
			// ����
			packet_info other_packetinfo;
			other_packetinfo.id = i_b->second.playerinfo.id;
			other_packetinfo.size = sizeof(i_b->second.playerinfo);
			other_packetinfo.sock = i_b->second.sock;
			other_packetinfo.type = sc_put_player;
			// ����
			player_info other_playerinfo = i_b->second.playerinfo;
			// buf�� packetinfo + playerinfo �����ϰ�.
			memcpy(buf, &other_packetinfo, sizeof(packet_info));
			memcpy(buf + sizeof(other_packetinfo), &other_playerinfo, sizeof(player_info));
			// buf�� messageBuffer�� �����ϰ�.
			memcpy(clients[client_s].buf, buf, sizeof(clients[client_s].buf));
			// ���̴� ����+���� ũ��.
			clients[client_s].wsabuf.len = sizeof(packet_info) + sizeof(player_info);
			// �������� ����ü ����. �ʱ�ȭ.
			memset(&(clients[client_s].overlapped), 0x00, sizeof(WSAOVERLAPPED));
			clients[client_s].overlapped.hEvent = (HANDLE)client_s;

			if (count > 2) {
				if (WSASend(client_s, &(clients[client_s].wsabuf), 1, &dataBytes, 0, &(clients[client_s].overlapped),
					send_nonrecv_callback) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
					}
				}
				count--;
			}
			else {
				if (WSASend(client_s, &(clients[client_s].wsabuf), 1, &dataBytes, 0, &(clients[client_s].overlapped),
					send_yesrecv_callback) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
					}
				}
			}
		}
	}
	else {
		clients[client_s].wsabuf.len = BUFSIZE;
		clients[client_s].wsabuf.buf = clients[client_s].buf;

		memset(&(clients[client_s].overlapped), 0x00, sizeof(WSAOVERLAPPED));
		clients[client_s].overlapped.hEvent = (HANDLE)client_s;

		if (WSARecv(client_s, &clients[client_s].wsabuf, 1,
			NULL/* ��������: �츮 null �� �ֱ�� ���ݾ�,, */, &flags, &(clients[client_s].overlapped), recv_callback) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("Error - Fail WSARecv(error_code : %d)\n", WSAGetLastError());
			}
		}
	}

}
void CALLBACK send_yesrecv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	if (dataBytes == 0)
	{
		closesocket(clients[client_s].sock);
		clients.erase(client_s);
		return;
	}  // Ŭ���̾�Ʈ�� closesocket�� ���� ���


	// WSASend(���信 ����)�� �ݹ��� ���
	clients[client_s].wsabuf.len = BUFSIZE;
	clients[client_s].wsabuf.buf = clients[client_s].buf;

	memset(&(clients[client_s].overlapped), 0x00, sizeof(WSAOVERLAPPED));
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;

	if (WSARecv(client_s, &clients[client_s].wsabuf, 1,
		NULL/* ��������: �츮 null �� �ֱ�� ���ݾ�,, */, &flags, &(clients[client_s].overlapped), recv_callback) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			printf("Error - Fail WSARecv(error_code : %d)\n", WSAGetLastError());
		}
	}
}

void CALLBACK send_nonrecv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

}

void show_allplayer()
{
	cout << endl << "------------ ���� �÷��̾� ���� --------------" << endl;
	cout << "���� ���� �� Ŭ���̾�Ʈ �� : " << clients.size() << endl;
	map<SOCKET, SOCKETINFO>::iterator i_b = clients.begin();
	map<SOCKET, SOCKETINFO>::iterator i_e = clients.end();
	for (map<SOCKET, SOCKETINFO>::iterator i_b = clients.begin();
		i_b != clients.end(); i_b++) {
		cout << i_b->second.playerinfo.id << "��° Ŭ���̾�Ʈ : "
			<< "��ǥ( "
			<< i_b->second.playerinfo.x << ", " << i_b->second.playerinfo.y << ", " << i_b->second.playerinfo.z << ")" << endl;
	}
	cout << "------------------------------------------------" << endl;
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
