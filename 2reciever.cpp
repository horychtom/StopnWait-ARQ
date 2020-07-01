#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <time.h>
#include "CRC.h"

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 992

#define LOAD_FROM "C:\\Users\\horyc\\Desktop\\to_send.jpg"
#define SAVE_TO "C:\\Users\\horyc\\Desktop\\recievedTest.jpg"
#define FILENAME "tosend"
#define RECEIVER
#define TARGET_PORT 14001
#define LOCAL_PORT 15000


void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

bool crcIsCorrect(char* buffer) {
	char data[BUFFERS_LEN - 4];
	//copy data from recieved buffer to data buffer for crc calculation
	for (int i = 0; i < (BUFFERS_LEN - 4); i++) {
		data[i] = buffer[i + 4];
	}
	//CRC CALCULATION
	uint32_t crc = CRC::Calculate(data, sizeof(data), CRC::CRC_32());

	char CRC[4];
	CRC[0] = (crc >> 24) & 0xFF;
	CRC[1] = (crc >> 16) & 0xFF;
	CRC[2] = (crc >> 8) & 0xFF;
	CRC[3] = crc & 0xFF;

	//comparing CRC stored in recieved buffer with ours
	for (int i = 0; i < 4; i++) {
		if (CRC[i] != buffer[i]) {
			return false;
		}
	}
	return true;
}

//**********************************************************************
int main()
{
	SOCKET socketS;

	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in from;

	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;


	socketS = socket(AF_INET, SOCK_DGRAM, 0);

	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0) {
		printf("Binding error!\n");
		getchar(); //wait for press Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN];
	char buffer_ack[1];
	//_____________________________________________________________________________

	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);

	FILE* to_file = fopen(SAVE_TO, "wb");

	//prepare
	strncpy(buffer_rx, "No data received.\n", BUFFERS_LEN);
	printf("Waiting for datagram ...\n");
	if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
		printf("Socket error!\n");
		getchar();
		return 1;
	}
	else {
		printf("Datagram: %s", buffer_rx);
	}


	int i = 0;
	int lastSuccesfull = 1;
	while (1) {
		if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
			printf("Socket error!\n");
			getchar();
			return 1;
		}
		//check if last packet
		if (buffer_rx[4] == 0) {
			break;
		}
		//crc
		if (crcIsCorrect(buffer_rx)) {
			buffer_ack[0] = 0x7f;

			if (buffer_rx[5] == lastSuccesfull) {
				printf("We already have this one, we dont want it, but it came ok\n");
				sendto(socketS, buffer_ack, 1, 0, (sockaddr*)&addrDest, sizeof(addrDest));
				continue;
			}
			else {
				printf("CRC GOOD and we dont have it yet %d\n", i++);
				fwrite((void*)(buffer_rx + 6), sizeof(char), BUFFERS_LEN - 6, to_file);
				sendto(socketS, buffer_ack, 1, 0, (sockaddr*)&addrDest, sizeof(addrDest));
				lastSuccesfull = buffer_rx[5];
			}
		}
		else {
			printf("CRC BAD\n");
			buffer_ack[0] = 0;
			sendto(socketS, buffer_ack, 1, 0, (sockaddr*)&addrDest, sizeof(addrDest));
		}

	}



	printf("Hotovo\n");
	fclose(to_file);
	closesocket(socketS);
	getchar(); //wait for press Enter
	return 0;
}