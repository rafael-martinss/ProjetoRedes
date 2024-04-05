#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "tcp.h"

static _sSocket_t m_socketId;

static void receiverCallback(uint8_t *buffer, uint16_t len)
{
	printf("Message received: %s\n", buffer);
	TCPSendData(m_socketId, "world!", strlen("world!!"));
}

static void connectionCallback(_sSocket_t socketClient, bool ConOrDiscon) {
	printf("Connection status of socket %d -> %s\n", (int)socketClient, 
			(ConOrDiscon) ? "Connected!" : "Disconnected..");
	TCPSendData(socketClient, "Hello!", strlen("Hello!!"));
}

/*
 * HELP MESSAGE
 */
#define MESSAGE_HELP    "\n"                                                \
                        "Usage: ./socket-connector [OPTION] <PARAM> ...\n"  \
                        " -c or --client\t\t: * Operate in client mode\n" 	\
                        " -s or --server\t\t: * Operate in server mode\n"   \
                        " -h or --help\t\t: * Command list\n"  				\
                        "\n"

int main(int argc, char *argv[])
{
	bool serverMode = false;
	int err;

	if (argc == 1) {
		printf("Missing configuration...\n\n");
		printf("%s", MESSAGE_HELP);
		exit(EXIT_FAILURE);
	}

	// Validate parameters
	for (int cont = 1; cont < argc; cont++)
	{
		if((strcmp(argv[cont], "-h") == 0) ||
			(strcmp(argv[cont], "--help") == 0)) {
			printf("%s", MESSAGE_HELP);
			exit(EXIT_SUCCESS);
		}
		else if((strcmp(argv[cont], "-c") == 0) ||
			(strcmp(argv[cont], "--client") == 0)) {
			serverMode = false;
		}
		else if((strcmp(argv[cont], "-s") == 0) ||
			(strcmp(argv[cont], "--server") == 0)) {
			serverMode = true;
		}
		else
		{
			printf("%s", MESSAGE_HELP);
			exit(EXIT_FAILURE);
		}
	}

	// Start the TCP layer
	if(err = TCPInit() != ERRCODE_NO_ERROR) {
		printf("Failure on TCP initialization: %d\n", err);
		return EXIT_FAILURE;
	}

	// Start the TCP connection
	if(err = TCPConnect(serverMode, &m_socketId, 
				  "127.0.0.1", 1234,
				  receiverCallback, connectionCallback) != ERRCODE_NO_ERROR ) {
		printf("Failure on %s, error: %d\n", (serverMode) ? "open connection" : "connection", err);
		return EXIT_FAILURE;
	}

	printf("Starting %s - socket %d\n", (serverMode) ? "server" : "client", (int)m_socketId);
	while (true)
	{
		sleep(1);
	}

	return EXIT_SUCCESS;
}
