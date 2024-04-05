/**
 ******************************************************************************
 * @file    tcp.c
 * @author  Rafael Martins
 ******************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "tcp.h"
#include "thread_wrapper.h"

/**
 * @note para checar o listen, via terminal linux:
 * lsof -i -P -n | grep LISTEN
 *
 */

/******************************************************************************
 * Defines
 *****************************************************************************/
// Periodo para rodar a thread de conexao (server) em ms.
#define TCP_CON_THREAD_REFRESH_PERIOD		(1000)

// Define o maximo de conexões pendentes
#define TCP_MAX_PENDING_CONNECTIONS			TCP_NUMBER_CLIENT_SOCKET

/******************************************************************************/
// Controle de estados de conexao (caso server)
enum _eTcpConnectionState
{
	_E_TCP_DISCONNECTED,
	_E_TCP_CONNECTED,
};

// Estrutura de socket interna
struct _sSocketClient
{
	_sSocket_t handle;
	enum _eTcpConnectionState eState;
	pthread_t xthrRecvID;
	bool bKillThreadRx;
	uint8_t buffer[TCP_BUFFER_SZ];
	CallbackReceiverTcp_t vCallbackTCPRx;
};

// Estrutura de socket interna
struct _sSocketServer
{
	sThread_t xthrServerID;
    struct _sSocketClient client[TCP_NUMBER_CLIENTS_TO_SERVER];
    uint8_t clientCount;
};

// estrutura de conexao
struct _sConnection
{
	bool bServer;
	_sSocket_t handle;
	enum _eTcpConnectionState eState;
	sThread_t xthrRecvID;
	bool bKillThreadRx;
	uint8_t buffer[TCP_BUFFER_SZ];
	CallbackReceiverTcp_t vCallbackTCPRx;
	CallbackConnection_t vCallbackTCPConnect;
	struct _sSocketServer server;
};

// Estrutura de trabalho
struct {
	struct _sConnection sConnection[TCP_NUMBER_CONNECTIONS];
    uint8_t counter;
} m_sTcpWork;

/*****************************************************************************/
/**
 * @brief Validacao para o proximo index valido para socket
 * @return indice disponivel ou TCP_NO_SOCKET caso nao haja mais espaco
 */
static int _TCPGetNextFreePosition(void);

/**
 * @brief Retorno do endereco que contem o ID de socket fornecido
 *
 * @param socketId Parametro de busca
 * @return Endereco da estrutura ou NULL, caso nao encontre
 */
static struct _sConnection* _TCPGetSocketStructPointer(_sSocket_t socketId);

/**
 * @brief Thread de conexao (server mode)
 *
 * @param arg
 */
void* _TCPThreadServer(void *arg);
/**
 * @brief Thread de recepcao de dados
 *
 * @param arg
 */
void* _TCPThreadRcve(void *arg);


/*****************************************************************************/
int TCPInit(void)
{
	int i;

	// Garante os sockets como inválidos
	memset(&m_sTcpWork, 0, sizeof(m_sTcpWork));
	for(i = 0; i < TCP_NUMBER_CONNECTIONS; i++)
	{
		m_sTcpWork.sConnection[i].handle = TCP_NO_SOCKET;
	}

	return ERRCODE_NO_ERROR;
}

//***************************************************************************
int TCPConnect(bool serverMode, _sSocket_t * socketId, char *ip, uint16_t port,
				   CallbackReceiverTcp_t receiveCb, CallbackConnection_t connectionCb)
{
	int ret;
	int idx;
	struct sockaddr_in server;

	// Valida se temos espaco
	idx = _TCPGetNextFreePosition();
	if(idx < 0)
	{
		ret = ERRCODE_TCP_NO_SPACE_FOR_CONNECTION;
		printf("Init socket error:%d\n", ret);
		goto exit;
	}

	m_sTcpWork.sConnection[idx].bServer = serverMode;

	if(serverMode)
	{
		m_sTcpWork.sConnection[idx].handle = TCP_NO_SOCKET;
		*socketId = socket(AF_INET , SOCK_STREAM , 0);
		if (*socketId < 0)
		{
			printf("Socket failed\n");
			ret = ERRCODE_TCP_SOCKET_FAILED;
			goto exit;
		}

		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons( port );
		if((ret = bind(*socketId,(struct sockaddr *)&server , sizeof(server))) < 0)
		{
			if(errno == EADDRINUSE)
			{
				printf("Address in use try cancel...\n");
				int tr=1;
				// kill "Address already in use" error message
				if (setsockopt(*socketId, SOL_SOCKET, SO_REUSEADDR,&tr,sizeof(int)) == -1)
				{
					printf("setsockopt failed");
					ret = ERRCODE_TCP_BIND_FAILED;
					goto close;
				}
				if((ret = bind(*socketId,(struct sockaddr *)&server , sizeof(server))) < 0)
				{
					ret = ERRCODE_TCP_BIND_FAILED;
					goto close;
				}
			}
			else
			{
				printf("Bind failed");
				ret = ERRCODE_TCP_BIND_FAILED;
				goto close;
			}
		}

		if(listen(*socketId, TCP_MAX_PENDING_CONNECTIONS))
		{
			printf("Listen failed");
			ret =  ERRCODE_TCP_LISTEN_FAILED;
			goto close;
		}

		m_sTcpWork.sConnection[idx].handle = *socketId;
		m_sTcpWork.sConnection[idx].vCallbackTCPRx = receiveCb;
		m_sTcpWork.sConnection[idx].vCallbackTCPConnect = connectionCb;
		m_sTcpWork.sConnection[idx].eState = _E_TCP_CONNECTED;

		ret = threadCreate(&m_sTcpWork.sConnection[idx].server.xthrServerID,
							"TCP-Server",
							_TCPThreadServer,
							&m_sTcpWork.sConnection[idx]);
		if(ret)
		{
			printf("Error thread Rcve");
			ret = ERRCODE_OS_FAILURE;
			goto close;
		}
	}
	else
	{
		m_sTcpWork.sConnection[idx].handle = TCP_NO_SOCKET;
		*socketId = socket(AF_INET , SOCK_STREAM , 0);
		if (*socketId < 0)
		{
			ret = ERRCODE_TCP_SOCKET_FAILED;
			goto exit;
		}

		server.sin_addr.s_addr = inet_addr(ip);
		server.sin_family 	   = AF_INET;
		server.sin_port   	   = htons( port );

		if (connect(*socketId , (struct sockaddr *)&server , sizeof(server)) < 0)
		{
			printf("Connect error..\n");
			ret = ERRCODE_TCP_CONNECTION_FAILED;
			goto close;
		}

		// Salva dados de controle do modulo
		m_sTcpWork.sConnection[idx].handle = *socketId;
		m_sTcpWork.sConnection[idx].vCallbackTCPConnect = connectionCb;
		m_sTcpWork.sConnection[idx].vCallbackTCPRx = receiveCb;
		m_sTcpWork.sConnection[idx].eState = _E_TCP_CONNECTED;

		ret = threadCreate(&m_sTcpWork.sConnection[idx].xthrRecvID,
							"Tcp-Rcve",
							_TCPThreadRcve,
							&m_sTcpWork.sConnection[idx].handle);
		if(ret)
		{
			printf("Erro thread Rcve - client\n");
			ret = ERRCODE_OS_FAILURE;
			goto close;
		}
	}

	// Sucesso, temos uma nova conexao
	m_sTcpWork.counter++;
	return ERRCODE_NO_ERROR;

close:
	close(*socketId);
exit:
	return ret;
}
//***************************************************************************
int TCPDisconnect(_sSocket_t socketId)
{
	int ret;
	struct _sConnection* psConnection;

	// Procura o socket na estrutura de trabalho
	psConnection = _TCPGetSocketStructPointer(socketId);
    if(psConnection == NULL)
    {
    	ret = ERRCODE_PARAMETRO_INVALIDO;
    	goto error;
    }

    if(psConnection->vCallbackTCPConnect != NULL)
    {
    	(*psConnection->vCallbackTCPConnect)(socketId, false);
    }

	ret = shutdown(socketId, SHUT_RDWR );
	ret |= close(socketId);
	if(ret)
	{
		printf("Close connection failed\n");
		ret = ERRCODE_TCP_DISCONNECT_FAILED;
		goto error;
	}

	if(psConnection->bServer)
	{
		if(psConnection->handle == socketId)
		{
			psConnection->server.clientCount = 0;
			psConnection->handle = TCP_NO_SOCKET;
			psConnection->bKillThreadRx = true;
			psConnection->eState = _E_TCP_DISCONNECTED;
		}
		else
		{
			int i;
			for(i = 0; i < TCP_NUMBER_CLIENTS_TO_SERVER; i++)
			{
				if(psConnection->server.client[i].handle == socketId)
				{
					psConnection->server.clientCount--;
					break;
				}
			}
			psConnection->server.client[i].handle = TCP_NO_SOCKET;
			psConnection->server.client[i].bKillThreadRx = true;
			psConnection->server.client[i].eState = _E_TCP_DISCONNECTED;
		}
	}
	else
	{
		psConnection->handle = TCP_NO_SOCKET;
		psConnection->bKillThreadRx = true;
		psConnection->eState = _E_TCP_DISCONNECTED;
	}

	printf("Desconexao do socket de ID %d\n", socketId);
	m_sTcpWork.counter--;

	ret = ERRCODE_NO_ERROR;

error:
	return ret;
}
//***************************************************************************
bool TCPIsConnected (_sSocket_t  socketId)
{
	struct _sConnection* psConnection;
	bool state;

	if(socketId == TCP_NO_SOCKET)
	{
		state = false;
		goto exit;
	}

	// Procura o socket na estrutura de trabalho
	psConnection = _TCPGetSocketStructPointer(socketId);
    if(psConnection == NULL)
    {
		state = false;
		goto exit;
    }
    state = (psConnection->eState == _E_TCP_CONNECTED) ? true : false ;

exit:
	return state;
}
//***************************************************************************
int TCPSendData(_sSocket_t socketId, char *p_pbuffer, uint16_t p_u16Len)
{
	int wr;
	int ret = ERRCODE_NO_ERROR;
	struct _sConnection* psConnection;

	// Procura o socket na estrutura de trabalho
	psConnection = _TCPGetSocketStructPointer(socketId);
    if(psConnection == NULL)
    {
    	return ERRCODE_PARAMETRO_INVALIDO;
    }

	wr = write(socketId, p_pbuffer, (size_t)p_u16Len);
	if(wr != p_u16Len)
	{
		ret = ERRCODE_TCP_WRITE_FAILED;
	}

	return ret;
}

/******************************************************************************
 * Local Functions code
 *****************************************************************************/
void* _TCPThreadServer(void *param)
{
	int connection;
	struct sockaddr_in client;
	struct _sConnection* psConnection = (struct _sConnection*)param;
	_sSocket_t socket;
	int ret;

	while(1)
	{
		connection = sizeof(struct sockaddr_in);
		socket  = accept(psConnection->handle, (struct sockaddr *)&client, (socklen_t*)&connection);
		if (socket > 0)
		{
			if(psConnection->server.clientCount >= TCP_NUMBER_CLIENTS_TO_SERVER)
			{
				shutdown(socket, SHUT_RDWR);
				close(socket);
				sleep(TCP_CON_THREAD_REFRESH_PERIOD);
				continue;
			}

			psConnection->server.client[psConnection->server.clientCount].handle = socket;
			psConnection->server.client[psConnection->server.clientCount].eState = _E_TCP_CONNECTED;
			ret = threadCreate(&psConnection->xthrRecvID,
								"TCP-Rcve",
								_TCPThreadRcve,
								&psConnection->server.client[psConnection->server.clientCount].handle);

			if(ret)
			{
				printf("Erro thread connect - server");
				goto exit;
			}

			if(psConnection->vCallbackTCPConnect != NULL)
				(*psConnection->vCallbackTCPConnect)(psConnection->server.client[psConnection->server.clientCount].handle, true);
			psConnection->server.clientCount++;
			m_sTcpWork.counter++;
		}
		else
			goto exit;
	}

	exit:
	// Se chegamos aqui, é uma excessao..
	threadExit();
	printf("End of thread Connection server..");
	sleep(1);
	exit(EXIT_FAILURE);

	return NULL;
}

//***************************************************************************
void* _TCPThreadRcve(void *param)
{
	_sSocket_t *psocket = (_sSocket_t*)param;;
	struct _sConnection* psConnection;
	int rd;

	do
	{
		// Procura o socket na estrutura de trabalho
		psConnection = _TCPGetSocketStructPointer(*psocket);
	    if(psConnection != NULL)
	    {
			rd = read(*psocket, psConnection->buffer , TCP_BUFFER_SZ);
			if(rd > 0)
			{
				(*psConnection->vCallbackTCPRx)(psConnection->buffer, rd);
			}
			else
			{
				if(psConnection == NULL)
				{
					psConnection = NULL;
				}
				// Ao ler ZERO, indicio de que tivemos uma desconexao
				TCPDisconnect(*psocket);
				threadExit();
			}
	    }
	} while(1);

	return NULL;
}

//***************************************************************************
static int _TCPGetNextFreePosition(void)
{
	int i;

	for(i = 0; i < TCP_NUMBER_CONNECTIONS; i++)
	{
		if(m_sTcpWork.sConnection[i].handle == TCP_NO_SOCKET)
		{
			return i;
		}
	}

	return TCP_NO_SOCKET;
}
//***************************************************************************
static struct _sConnection* _TCPGetSocketStructPointer(_sSocket_t socketId)
{
	int i;

	if(socketId == TCP_NO_SOCKET)
		return NULL;

    for(i = 0; i < TCP_NUMBER_CONNECTIONS; i++)
    {
        if(m_sTcpWork.sConnection[i].bServer)
        {
        	if(m_sTcpWork.sConnection[i].handle == socketId)
			{
        		 return &m_sTcpWork.sConnection[i];
        	}
        	else
        	{
    			int j;
    			for(j = 0; j < TCP_NUMBER_CLIENTS_TO_SERVER; j++)
    			{
    				if(m_sTcpWork.sConnection[j].server.client[j].handle == socketId)
    				{
    					 return &m_sTcpWork.sConnection[i];
    				}
    			}
        	}
        }
        else
        {
        	if(m_sTcpWork.sConnection[i].handle == socketId)
			{
        		 return &m_sTcpWork.sConnection[i];
			}
        }
    }
   return NULL;
}
