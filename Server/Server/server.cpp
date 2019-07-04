#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "../../Headers/Include.h"
#include <map>
#pragma comment(lib, "ws2_32")

using namespace std;


// ���� ���� ������ ���� ����ü�� ����
struct SOCKETINFO
{
	WSAOVERLAPPED overlapped; // overlapped ����ü
	WSABUF wsabuf;	// WSABUF ����ü. ���� ��ü�� �ƴ�, ������ ������
	SOCKET sock;
	char buf[BUFSIZE + 1];	// ���� ���α׷� ����. ���� ���۰� �� �޽��� ����.
	player_info playerinfo = {};
	bool connected = false;
	bool is_recv = true;
};

// map<SOCKET, SOCKETINFO> clients;
SOCKETINFO clients[NUM_OF_PLAYER];

// �񵿱� ����� ���۰� ó�� �Լ�
void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_csputplayer_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);

// �Լ���
void send_packet(char, char*);
void do_recv(char id);

void err_quit(const char *msg);
void err_display(const char *msg);
void show_allplayer();



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
		cout << "[Ŭ���̾�Ʈ ����] IP �ּ� (" << inet_ntoa(client_addr.sin_addr) << "), ��Ʈ ��ȣ (" << ntohs(client_addr.sin_port) << ")" << endl;
		// ���̵� �ο�.
		int new_id = -1;
		for (int i = 0; i < NUM_OF_PLAYER; ++i) {
			if (false == clients[i].connected) {
				clients[i].connected = true;
				new_id = i;
				break;
			}
		}
		// ������ �ɼ��� ����.
		bool NoDelay = TRUE;
		setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (const char FAR*)&NoDelay, sizeof(NoDelay));

		// Ŭ���̾�Ʈ ����ü�� ���� �Է�.
		{
			clients[new_id] = SOCKETINFO{};
			memset(&clients[new_id], 0x00, sizeof(SOCKETINFO));
			clients[new_id].playerinfo.id = new_id;
			clients[new_id].playerinfo.connected = true;
			clients[new_id].sock = client_sock;
			clients[new_id].playerinfo.x = INITPOSITION_X;
			clients[new_id].playerinfo.z = INITPOSITION_Z;
			// clients[new_id].playerinfo.type = player_type(new_id % 2);
			clients[new_id].playerinfo.animationSet = 0;
			clients[new_id].wsabuf.len = BUFSIZE;
			clients[new_id].wsabuf.buf = clients[new_id].buf;
			clients[new_id].overlapped.hEvent = (HANDLE)clients[new_id].sock;
			clients[new_id].connected = true;
			flags = 0;
		}
		


		// �α��� �Ǿ��ٴ� ��Ŷ�� ������.
		{
			char buf[BUFSIZE];
			// (����)
			packet_info packetinfo;
			packetinfo.id = new_id;
			packetinfo.size = sizeof(player_info);
			packetinfo.type = sc_notify_yourinfo;
			memcpy(buf, &packetinfo, sizeof(packetinfo));
			// (����)
			memcpy(buf + sizeof(packetinfo), &(clients[new_id].playerinfo), sizeof(player_info));
			// ����
			send_packet(new_id, buf);
		}

		// new_id�� ������ �ٸ� Ŭ���̾�Ʈ���� �˸���.
		for (int i = 0; i < NUM_OF_PLAYER; ++i) {
			if (!clients[i].connected) continue;  // ����� Ŭ���̾�Ʈ���Ը� ������.
			if (i == new_id) continue;
			char buf[BUFSIZE];
			// (����)
			packet_info packetinfo;
			packetinfo.id = new_id;
			packetinfo.size = sizeof(player_info);
			packetinfo.type = sc_put_player;
			memcpy(buf, &packetinfo, sizeof(packetinfo));
			// (����)
			memcpy(buf + sizeof(packetinfo), &(clients[new_id].playerinfo), sizeof(player_info));
			// ����
			send_packet(new_id, buf);

		}

		// �ٸ� Ŭ���̾�Ʈ�� ���縦 new_id Ŭ���̾�Ʈ���� �˸���.
		for (int i = 0; i < NUM_OF_PLAYER; ++i) {
			if (false == clients[i].connected) continue;
			if (i == new_id) continue; // �����Դ� ������ �ʴ´�.

			char buf[BUFSIZE];
			// (����)
			packet_info packetinfo;
			packetinfo.id = i;
			packetinfo.size = sizeof(player_info);
			packetinfo.type = sc_put_player;
			memcpy(buf, &packetinfo, sizeof(packetinfo));
			// (����)
			memcpy(buf + sizeof(packetinfo), &(clients[i].playerinfo), sizeof(player_info));
			// ����
			send_packet(new_id, buf);
		}

		do_recv(new_id);
		show_allplayer();
	}

	// closesocket()
	closesocket(listen_sock);
	// ���� ����
	WSACleanup();

	return 0;
}


void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	// �������忡�� ������ �о�´�.
	SOCKETINFO *socketInfo = (struct SOCKETINFO *)overlapped;
	SOCKET client_sock = reinterpret_cast<int>(overlapped->hEvent);

	int id = socketInfo->playerinfo.id;

	if (id > NUM_OF_PLAYER)
		return;

	if (dataBytes == 0) {
		// Ŭ���̾�Ʈ ���� ����.
		closesocket(clients[id].sock);
		clients[id].connected = false;
		return;
	}


	// ���� ���� ��Ŷ.
	player_info playerinfo;
	packet_info packetinfo;
	memcpy(&packetinfo, clients[id].buf, sizeof(packetinfo));
	int fromid = packetinfo.id;


	// ���� ���� ��Ŷ.
	switch (packetinfo.type) {
	case cs_move:
	{
		// 1. cs_move ��Ŷ�� ������ Ŭ���̾�Ʈ�� ������ �޾ƿ´�.
		{
			char buf[BUFSIZE];
			memcpy(&playerinfo, clients[fromid].buf + sizeof(packetinfo), sizeof(player_info));
			clients[fromid].playerinfo = playerinfo;
		}

		// 2. �ٸ� ���� ���� Ŭ���̾�Ʈ���� ������ Ŭ���̾�Ʈ�� ������ ������.
		{
			char buf[BUFSIZE];
			// (����)
			memset(&packetinfo, 0x00, sizeof(packetinfo));
			packetinfo.id = fromid;
			packetinfo.size = sizeof(playerinfo);
			packetinfo.type = sc_notify_playerinfo;
			memcpy(buf, &packetinfo, sizeof(packetinfo));
			// (����)
			memcpy(buf + sizeof(packetinfo), &(clients[fromid].playerinfo), sizeof(player_info));
			// ����
			for (int i = 0; i < NUM_OF_PLAYER; ++i) {
				if (!clients[i].connected) continue;
				if (i == fromid) continue;
				send_packet(i, buf);
			}

		}

		// 3. ������ Ŭ���̾�Ʈ�� ������ �ٽ� Recv�� �����Ѵ�.
		{
			do_recv(fromid);
		}

		// 4. ���� ���� ���� Ŭ���̾�Ʈ �÷��̾� ���� ���.
		show_allplayer();
	}
	break;
	case cs_put_playertype:
	{
		// 1. cs_put_playertype ��Ŷ�� ������ Ŭ���̾�Ʈ�� "type" ������ �޾ƿ´�.
		{
			char buf[BUFSIZE];
			memcpy(&playerinfo, clients[fromid].buf + sizeof(packetinfo), sizeof(player_info));
			clients[fromid].playerinfo.type = playerinfo.type;
		}
		// 2. ������ Ŭ���̾�Ʈ�� ������ �ٽ� Recv�� �����Ѵ�.
		{
			do_recv(fromid);
		}
	}
	break;
	case cs_snake_is_dead:
	{
		// 1. ���� ������ id�� �޾ƿ´�.
		int monsterId = -1;
		char buf[BUFSIZE];
		memcpy(&monsterId, clients[fromid].buf + sizeof(packetinfo), sizeof(int));
		
		// 2. �ٸ� Ŭ���̾�Ʈ���Ե� ���� ������ id�� �˸���.
		memset(&packetinfo, 0x00, sizeof(packetinfo));
		packetinfo.id = fromid;
		packetinfo.size = sizeof(int);
		packetinfo.type = sc_snake_is_dead;
		memcpy(buf, &packetinfo, sizeof(packetinfo));
		memcpy(buf + sizeof(packetinfo), &(monsterId), sizeof(int));
		for (int i = 0; i < NUM_OF_PLAYER; ++i) {
			if (!clients[i].connected) continue;
			if (i == fromid) continue;
			send_packet(i, buf);
		}

		// 3. ������ Ŭ���̾�Ʈ ������ �ٽ� recv�� �����Ѵ�.
		do_recv(fromid);

	}
	break;
	case cs_notify_snakeinfo:
	{
		// 1. ���� ������ info�� �޾ƿ´�.
		snake_info s_info;
		char buf[BUFSIZE];
		memcpy(&s_info, clients[fromid].buf + sizeof(packetinfo), sizeof(snake_info));

		// 2. �ٸ� Ŭ���̾�Ʈ���Ե� ������ info�� �˸���.
		memset(&packetinfo, 0x00, sizeof(packetinfo));
		packetinfo.id = fromid;
		packetinfo.size = sizeof(snake_info);
		packetinfo.type = sc_notify_snakeinfo;
		memcpy(buf, &packetinfo, sizeof(packetinfo));
		memcpy(buf + sizeof(packetinfo), &(s_info), sizeof(snake_info));
		for (int i = 0; i < NUM_OF_PLAYER; ++i) {
			if (!clients[i].connected) continue;
			if (i == fromid) continue;
			send_packet(i, buf);
		}

		// 3. ������ Ŭ���̾�Ʈ ������ �ٽ� recv�� �����Ѵ�.
		do_recv(fromid);

	}
	break;
	default:
		do_recv(fromid);
		break;
	}


}

void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKETINFO *socketInfo = (struct SOCKETINFO *)overlapped;
	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);
	int id = socketInfo->playerinfo.id;

	delete socketInfo;

	if (dataBytes == 0) {
		cout << id << "��° ������ �ݽ��ϴ�." << endl;
		closesocket(clients[id].sock);
		clients[id].connected = false;
		return;
	}

}

void CALLBACK send_csputplayer_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
}

void disconnect_client(char id) {

}

void send_packet(char client, char* buf)
{
	SOCKETINFO* socketinfo = new SOCKETINFO;
	socketinfo->playerinfo = clients[client].playerinfo;
	socketinfo->wsabuf.len = BUFSIZE;
	socketinfo->wsabuf.buf = socketinfo->buf;
	memcpy(socketinfo->buf, buf, sizeof(socketinfo->buf));
	ZeroMemory(&(socketinfo->overlapped), sizeof(socketinfo->overlapped));
	DWORD recvBytes = 0;

	if (WSASend(clients[client].sock, &(socketinfo->wsabuf), 1, &recvBytes, 0, &(socketinfo->overlapped),
		send_callback) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			printf("Error - Fail WSASend(error_code : %d)\n", WSAGetLastError());
		}
	}
}




void do_recv(char id)
{
	DWORD flags = 0;

 	clients[id].is_recv = true;
	if (WSARecv(clients[id].sock, &clients[id].wsabuf, 1,
		NULL, &flags, &(clients[id].overlapped), recv_callback))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			cout << "Error - IO pending Failure\n";
			while (true);
		}
	}
	//else {
	//	cout << "Non Overlapped Recv return.\n";
	//	while (true);
	//}
}


void show_allplayer()
{
	cout << endl << "------------ ���� �÷��̾� ���� --------------" << endl;
	int count = 0;

	for (int i = 0; i < NUM_OF_PLAYER; ++i) {
		if (clients[i].connected == true) {
			count++;
			cout << clients[i].playerinfo.id << "��° Ŭ���̾�Ʈ : "
				<< "��ǥ( "
				<< clients[i].playerinfo.x << ", " << clients[i].playerinfo.y << ", " << clients[i].playerinfo.z << ")"
				// << ", �ִϸ��̼Ǽ�(" << clients[i].playerinfo.animationSet << ")"
				// << ", xmf3Look(" << clients[i].playerinfo.l_x << ", " << clients[i].playerinfo.l_y << ", " << clients[i].playerinfo.l_z << ")"
				// << ", xmf3Right(" << clients[i].playerinfo.r_x << ", " << clients[i].playerinfo.r_y << ", " << clients[i].playerinfo.r_z << ")"
				// << ", piggybackstate(" << clients[i].playerinfo.piggybackstate << ")"
				<< ", playertype(" << clients[i].playerinfo.type << ")"
				<< endl;
		}
	}
	cout << "���� ���� �� Ŭ���̾�Ʈ �� : " << count << endl;
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

