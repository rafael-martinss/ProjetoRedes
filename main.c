#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "tcp.h"
#include "mpu6050.h"

static _sSocket_t m_socketId;

#ifndef CLIENT_MODE
static _sSocket_t m_clientSocketId;
#endif

static float m_accel_x, m_accel_y, m_accel_z = 0;
static float m_gyro_x, m_gyro_y, m_gyro_z = 0;

const char kAccelHeaderMsg[] = "Accel: ";
const char kGyroHeaderMsg[] = "Gyro: ";
const char kBodySeed = ':';

static void receiverCallback(uint8_t *buffer, uint16_t len)
{
#ifdef CLIENT_MODE
	printf("Message received: %s\n", buffer);
#else
	char sendBuffer[1024] = {0};
	memset(sendBuffer, 0, sizeof(sendBuffer));
	// Validade which was the received message
	if (strstr(buffer, kAccelHeaderMsg) != NULL) {
		char *content = strchr(buffer, kBodySeed);
		float x, y, z = 0;
		if(content != NULL) {
			sscanf(content, ": %f-%f-%f", &x, &y, &z);
			printf("<Accel message>: (x %f, y %f, z %f)\n", x, y, z);
			sprintf(sendBuffer, "<Delta on Accel>: (x %.2f, y %.2f, z %.2f)", m_accel_x - x, m_accel_y - y, m_accel_z - z);
			m_accel_x = x;
			m_accel_y = y;
			m_accel_z = z;
		}
	} else if (strstr(buffer, kGyroHeaderMsg) != NULL) {
		char *content = strchr(buffer, kBodySeed);
		float x, y, z = 0;
		if (content != NULL)
		{
			sscanf(content, ": %f-%f-%f", &x, &y, &z);
			printf("<Gyro message>: (x %f, y %f, z %f)\n", x, y, z);
			sprintf(sendBuffer, "<Delta on Gyro>: (x %.2f, y %.2f, z %.2f)", m_gyro_x - x, m_gyro_y - y, m_gyro_z - z);
			m_gyro_x = x;
			m_gyro_y = y;
			m_gyro_z = z;
		}
	} else {
		printf("<Unknown message>\n");
		return;
	}
	TCPSendData(m_clientSocketId, sendBuffer, strlen(sendBuffer));
#endif
}

static void connectionCallback(_sSocket_t socketClient, bool ConOrDiscon) {
#ifndef CLIENT_MODE
	m_clientSocketId = (ConOrDiscon) ? socketClient : 0;
#endif
	printf("Connection status of socket %d -> %s\n", (int)socketClient, 
			(ConOrDiscon) ? "Connected!" : "Disconnected..");
}

#ifdef CLIENT_MODE
static bool get_sensor_data() {
	static float x, y, z = 0;
	bool must_update = false;

	mpu6050_get_accel(&x, &y, &z);
	printf("%s%f, %f, %f\n", kAccelHeaderMsg, x, y, z);
	if (x != m_accel_x || y != m_accel_y || z != m_accel_z)
	{
		m_accel_x = x;
		m_accel_y = y;
		m_accel_z = z;
		must_update = true;
	}

	mpu6050_get_gyro(&x, &y, &z);
	printf("%s%f, %f, %f\n",kGyroHeaderMsg, x, y, z);
	if( x != m_gyro_x || y != m_gyro_y || z != m_gyro_z) {
		m_gyro_x = x;
		m_gyro_y = y;
		m_gyro_z = z;
		must_update =  true;
	}

	return must_update;
}


void send_notification(){
	char buffer[200] = {0};

	if (get_sensor_data())
	{
		sprintf(buffer, "Accel: %f-%f-%f", m_accel_x, m_accel_y, m_accel_z);
		TCPSendData(m_socketId, buffer,  strlen(buffer));

		sprintf(buffer, "Gyro: %f-%f-%f", m_gyro_x, m_gyro_y, m_gyro_z);
		TCPSendData(m_socketId, buffer, strlen(buffer));
	}
}
#endif

/*
 * HELP MESSAGE
 */
#define MESSAGE_HELP    "\n"                                                \
                        "Usage: ./socket-connector [OPTION] <PARAM> ...\n"  \
                        " -i or --ip\t\t: * IP for connection (formatted as AAA.BBB.CCC.DDD\n" \
                        " -h or --help\t\t: * Command list\n"  				\
                        "\n"

#define SERVER_PORT			1234
#define SERVER_DEFAULT_IP  "192.168.0.23"

int main(int argc, char *argv[])
{
	int err;
	char ip[16] = SERVER_DEFAULT_IP;
#ifdef CLIENT_MODE
	bool serverMode = false;
#else
	bool serverMode = true;
#endif

#ifdef CLIENT_MODE
	if (argc == 1) {
		printf("Missing configuration...\n\n");
		printf("%s", MESSAGE_HELP);
	}

	// Validate parameters
	for (int cont = 1; cont < argc; cont++)
	{
		if((strcmp(argv[cont], "-h") == 0) ||
			(strcmp(argv[cont], "--help") == 0)) {
			printf("%s", MESSAGE_HELP);
			exit(EXIT_SUCCESS);
		}
		else if((strcmp(argv[cont], "-i") == 0) ||
			(strcmp(argv[cont], "--ip") == 0)) {
			strcpy(ip, argv[++cont]);
		}
		else
		{
			printf("%s", MESSAGE_HELP);
			exit(EXIT_FAILURE);
		}
	}

	// Initialize the sensor
	mpu6050_init();
#endif

	// Start the TCP layer
	if(err = TCPInit() != ERRCODE_NO_ERROR) {
		printf("Failure on TCP initialization: %d\n", err);
		return EXIT_FAILURE;
	}

	// Start the TCP connection
	if(err = TCPConnect(serverMode, &m_socketId, 
				  ip, SERVER_PORT,
				  receiverCallback, connectionCallback) != ERRCODE_NO_ERROR ) {
		printf("Failure on %s, error: %d\n", (serverMode) ? "open connection" : "connection", err);
		return EXIT_FAILURE;
	}

	printf("Starting %s - socket %d\n", (serverMode) ? "server" : "client", (int)m_socketId);

	// Collecting data, if applicable
	while (true)
	{
#ifdef CLIENT_MODE
		// Read sensor and validate for notification
		// The sensor reading is only available for client!
		send_notification();
#endif
		sleep(2);
	}

	return EXIT_SUCCESS;
}
