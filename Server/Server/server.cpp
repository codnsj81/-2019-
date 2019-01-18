#include <winsock2.h>
#include <iostream>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define MAX_BUFFER 1024
#define SERVER_PORT 3500

// Ŭ���̾�Ʈ�� ������ ������ �� �ʿ��� ������.
struct SOCKETINFO
{
	WSAOVERLAPPED overlapped;	 // overlapped IO ����ü
	WSABUF dataBuffer; // ���� ��ü�� �ƴ� ������ ������
	SOCKET socket;
	char messageBuffer[MAX_BUFFER]; // ���� ���۰� ���� �޽��� ����
	int receiveBytes; // �� �޾Ұ�
	int sendBytes; // �� ���´���
};

// overlapped : send, recv ���� call by reference�� �����.
void CALLBACK callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD InFlags);

int main()
{
	WSADATA WSAData;

	// 0. Winsock Start
	{
		if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
			cout << "Error - Can not load 'winsock.dll' file" << endl;
			return 1;
		}
	}

	// 1. ����� ���� ����
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	{
		if (listenSocket == INVALID_SOCKET) {
			cout << "Error - Invalid socket" << endl;
			return 1;
		}
	}

	/// ���� ���� ��ü ����
	SOCKADDR_IN serverAddr;
	{
		memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
		serverAddr.sin_family = PF_INET;
		serverAddr.sin_port = htons(SERVER_PORT);
		serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	}

	// 2. ���� ����
	if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
		// 6. ���� ����
		cout << "Error - Fail bind" << endl;
		closesocket(listenSocket);
		// winsock end
		WSACleanup();
		return 1;
	}

	// 3. ���� ��⿭ ����
	// ���� 2����
	if (listen(listenSocket, 2) == SOCKET_ERROR) {
		// 6. ���� ����
		cout << "Error - Fail listen" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	/// �ʿ��� ������
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);
	SOCKET clientSocket;
	SOCKETINFO* socketInfo;
	DWORD receiveBytes;
	DWORD flags;

	// ������ ���ؼ� ��� accept - ���� ����� �����..
	while (true)
	{
		// ���ϰ� �ּ� ����ü �ֱ�.
		/// accept�ϸ� Ŭ���̾�Ʈ�� ���� �ǰ�, Ŭ���̾�Ʈ�� �ּ� ����ü ������ �޾ƿͼ� �ּ� ���� ����.
		clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) {
			cout << "Error - Accept Failure" << endl;
			return 1;
		}

		// Ŭ���̾�Ʈ�� ������ ����� �;� �Ѵ�.
		socketInfo = (struct SOCKETINFO*)malloc(sizeof(struct SOCKETINFO));
		ZeroMemory(socketInfo, sizeof(struct SOCKETINFO));
		socketInfo->socket = clientSocket;
		socketInfo->dataBuffer.len = MAX_BUFFER;
		socketInfo->dataBuffer.buf = socketInfo->messageBuffer;
		flags = 0;

		// �Ѱ� �ְ�, �޾Ƽ� ����.
		socketInfo->overlapped.hEvent = (HANDLE)socketInfo->socket;
		
		// ��ø ������ �����ϰ�, �Ϸ�� ����� �Լ��� �Ѱ� �ش�.
		// recv �ϰ� �޾ƿ� �����͸� ó���ؾ� �ϴµ� �ٷ� ������? -> overlapped IO ��.
			// ���� �����Ͱ� ���ۿ� ������ �� ���߿�.
		if (WSARecv(socketInfo->socket, &socketInfo->dataBuffer, 1, &receiveBytes, &flags,
			&(socketInfo->overlapped), callback)) {

			// overlapped IO�� pending ������ ���;� �Ѵ�.
			// ������ �����͸� �̹� �о���� ����.
			if (WSAGetLastError() != WSA_IO_PENDING) {
				cout << "Error - IO Pending Failure" << endl;
				return 1;
			}
		}
	}

	// 6-2. ���� ���� ����.
	closesocket(listenSocket);

	// Winsock End.
	WSACleanup();

	return 0;
}


void CALLBACK callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD InFlags)
{
	struct SOCKETINFO *socketInfo;
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	// recv ���� ��, �� ��° �������� �� ������ overlapped ����ü�� ���� �˾ƿ´�.
	/// ������ �� ���� overlapped ����ü�� Ÿ�� ĳ���� �Ѵ�.
	/// SOCKETINFO�� ����ü�� ù ��� ������ overlapped�̱� ������..! �ļ���.
	/// callback �Լ��� ���ڷ� ������ �� �߿� ������ �����Ƿ�.
	socketInfo = (struct SOCKETINFO*)overlapped;

	// recv ���� ��, 0 ����Ʈ�� �о��ٸ�?
	//  -> ���ӵ� Ŭ���̾�Ʈ���� ���� ������ �� ���̴�. �� CloseSocket() ȣ�� �� ��.
	if (dataBytes == 0) {
		// Ŭ�󿡼� �����µ� ���� ��� ���� �ʿ� ����. ������ �ش�.
		closesocket(socketInfo->socket);
		free(socketInfo);
		return;
	}
	

	if (socketInfo->receiveBytes == 0) {
		// WSARecv(���� ��⿡ ����) �� callback�� ���.
		/// '���� ����'���� ���� �����Ͱ� 0�� ũ���� ó�� ���� ����. �� ���� callback�� ���.
		socketInfo->receiveBytes = dataBytes; // �̸�ŭ �޾����ϱ� �̸�ŭ ����
		socketInfo->sendBytes = 0; // ���� �ϳ��� �� ������ ������
		socketInfo->dataBuffer.buf = socketInfo->messageBuffer; // ������
		socketInfo->dataBuffer.len = socketInfo->receiveBytes; // ������ ���� ������ ��ŭ�� ������ �Ѵ�.

		cout << "TRACE - Receive message : " << socketInfo->messageBuffer <<
			" ( " << dataBytes << "bytes )" << endl;

		// send, recv �ϱ� ���� ������ 0���� �ʱ�ȭ �� �־��.
		/// overlapped IO�� �����ϰ�, recv�� ����Ǿ����Ƿ�, �� recv�� �Ϸ��� overlapped ����ü�� 0���� �̸� �ʱ�ȭ.
		/// ���� memset���� ZeroMemory�� ����.
		ZeroMemory(&(socketInfo->overlapped), sizeof(WSAOVERLAPPED));

		// Ŭ���̾�Ʈ���� �� ���� �״�� ����. -> �� �ʿ� ����.
		if (WSASend(socketInfo->socket, &(socketInfo->dataBuffer), 1, &sendBytes, 0, &(socketInfo->overlapped), callback) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				cout << "Error - Fail WSASend (error_code : " << WSAGetLastError() << ")" << endl;
			}
		}
	}
	else { // send callback
		
		// WSASend(���信 ����) callback�� ���.
		socketInfo->sendBytes += dataBytes; 
		socketInfo->receiveBytes = 0;
		socketInfo->dataBuffer.len = MAX_BUFFER;
		socketInfo->dataBuffer.buf = socketInfo->messageBuffer;

		cout << "TRACE - Send Message : " << socketInfo->messageBuffer << "( " << dataBytes << "bytes )" << endl;

		// overlapped IO�� �����ϰ�, recv�� ����Ǿ����Ƿ�,
		// �� recv�� �Ϸ��� overlapped ����ü�� 0���� �̸� �ʱ�ȭ �Ѵ�.
		ZeroMemory(&(socketInfo->overlapped), 0, sizeof(WSAOVERLAPPED));

		if (WSARecv(socketInfo->socket, &socketInfo->dataBuffer, 1, &receiveBytes,
			&flags, &(socketInfo->overlapped), callback) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				cout << "Error - Fail WSARecv (error_code : " << WSAGetLastError() << ")" << endl;
			}
		}
	}

}