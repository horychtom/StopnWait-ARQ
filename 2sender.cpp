
#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include "CRC.h"

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 992

#define LOAD_FROM "C:\\Users\\horyc\\Desktop\\sendit2.jpg"
#define SAVE_TO "C:\\Users\\horyc\\Desktop\\recievedTest.jpg"
#define FILENAME "FILENAME.jpg"
#define SENDER

#define TARGET_PORT 14000
#define LOCAL_PORT 15001



void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void computeCRC(char* buffer) {

	char data[BUFFERS_LEN - 4];
	for (int i = 0; i < (BUFFERS_LEN - 4); i++) {
		data[i] = buffer[i + 4];
	}

	uint32_t crc = CRC::Calculate(data, sizeof(data), CRC::CRC_32());

	buffer[0] = (crc >> 24) & 0xFF;
	buffer[1] = (crc >> 16) & 0xFF;
	buffer[2] = (crc >> 8) & 0xFF;
	buffer[3] = crc & 0xFF;

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

	char buffer_tx[BUFFERS_LEN];
	char buffer_ack[1];


	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);

	//select() setup



	//_____________________________________________________________________________
	//____________________THE CODE STARTS HERE_____________________________________

	//sending filename
	strncpy(buffer_tx, FILENAME, BUFFERS_LEN); //put some data to buffer
	printf("Sending packet.\n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));

	printf("OPENING %s \n", LOAD_FROM);
	FILE* from_file = fopen(LOAD_FROM, "rb");

	if (from_file == NULL) {
		printf("ERROR cannot open from_file");
		return -1;
	}

	buffer_tx[4] = 1;

	printf("Sending packets.\n");

	int i = 0;
	int seq = 0;

	while (1) {

		fread((buffer_tx + 6), sizeof(char), BUFFERS_LEN - 6, from_file);
		buffer_tx[5] = (seq++) % 2;
		computeCRC(buffer_tx);
		sendto(socketS, buffer_tx, BUFFERS_LEN, 0, (sockaddr*)&addrDest, sizeof(addrDest));

		while (1) {
			fd_set reads;
			struct timeval timeout;
			FD_ZERO(&reads);
			FD_SET(socketS, &reads);
			timeout.tv_sec = 10;
			timeout.tv_usec = 0;

			int rv = select(socketS, &reads, NULL, NULL, &timeout);
			if (rv == SOCKET_ERROR) {
				printf("Socket error!\n");
				getchar();
				return 1;
			}
			//timeout
			else if (rv == 0) {
				printf("Nothing happend so imma send it again\n");
				sendto(socketS, buffer_tx, BUFFERS_LEN, 0, (sockaddr*)&addrDest, sizeof(addrDest));
				continue;
			}
			//something CAME back, lets check it
			else {
				if (recvfrom(socketS, buffer_ack, sizeof(buffer_ack), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
					printf("Socket error!\n");
					getchar();
					return 1;
				}
				if (buffer_ack[0] == 0x7f) {
					printf("It worked, lets move on %d\n", i++);
					break;
				}
				else {
					sendto(socketS, buffer_tx, BUFFERS_LEN, 0, (sockaddr*)&addrDest, sizeof(addrDest));
				}
			}
		}

		if (feof(from_file)) {
			break;
		}

	}


	buffer_tx[4] = 0;
	sendto(socketS, buffer_tx, BUFFERS_LEN, 0, (sockaddr*)&addrDest, sizeof(addrDest));


	closesocket(socketS);
	fclose(from_file);
	printf("ALL SEND\n");

	getchar(); //wait for press Enter
	return 0;
}
