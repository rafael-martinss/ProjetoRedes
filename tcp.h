/**
 ******************************************************************************
 * @file    tcp.h
 * @author  Rafael Martins
 ******************************************************************************
 */

#ifndef TCP_H_
#define TCP_H_

#include <stdbool.h>
#include <stdint.h>

// Tamanho do buffer TCP
#define TCP_BUFFER_SZ					1500

// Numero maximo de servidores TCP para administrar
#define TCP_NUMBER_SERVER_SOCKET		1

// Numero maximo de clients que os servers irao administrar
#define TCP_NUMBER_CLIENTS_TO_SERVER 	1

// Numero maximo de clients TCP para administrar
#define TCP_NUMBER_CLIENT_SOCKET		1

// Numero maximo de conexoes TCP para administrar
#define TCP_NUMBER_CONNECTIONS			(TCP_NUMBER_SERVER_SOCKET + TCP_NUMBER_CLIENT_SOCKET)

// Retorno para socket vago
#define TCP_NO_SOCKET					-1

/*****************************************************************************/
enum
{
	ERRCODE_NO_ERROR,		  				      // Sucesso na operacao
	ERRCODE_PARAMETRO_INVALIDO,		  			  //
	ERRCODE_OS_FAILURE,		  			 		  //
	ERRCODE_TCP_NO_SPACE_FOR_CONNECTION,		  // Maximo de conexoes atingidas
	ERRCODE_TCP_CONNECTION_FAILED,				  // Falha na conexao
	ERRCODE_TCP_SOCKET_FAILED,				  	  // Falha no socket
	ERRCODE_TCP_BIND_FAILED,				  	  // Falha no bind
	ERRCODE_TCP_LISTEN_FAILED,				  	  // Falha no listen
	ERRCODE_TCP_ACCEPT_FAILED,				  	  // Falha no listen de conexoes
	ERRCODE_TCP_WRITE_FAILED,				  	  // Falha na escrita correta de dados
	ERRCODE_TCP_DISCONNECT_FAILED,				  // Falha na desconexao
};

// Definição do handle dos dados de conexão (definição para maior compatibilidade genérica)
typedef int _sSocket_t;

/**
 * @brief Callback de recepção de dados TCP
 * @param buffer: Buffer de leitura
 * @param len: Tamanho do buffer
 */
typedef void (*CallbackReceiverTcp_t) (uint8_t *buffer, uint16_t len);
/**
 * @brief  Callback de conexao de um client ao servidor TCP
 * @param socketClient - Socket do client
 * @param ConOrDiscon - Se true, houve uma conexao, false uma desconexao
 */
typedef void (*CallbackConnection_t) (_sSocket_t socketClient, bool ConOrDiscon);

/******************************************************************************/
/**
 * @brief Inicializacao do modulo, limpeza de variaveis
 *
 * @return Codigo de erro
 */
int TCPInit(void);
//***************************************************************************
/**
 * @brief Conexao a um ponto. Para o modo client, necessitamos do enderedo IP
 *
 * @param serverMode - Indicativo para operar modo client(false) ou server (true)
 * @param socket - Ponteiro para armazenar o socket criado
 * @param ip - string com o IP para conexao
 * @param port: Porta de conexao
 * @param receiveCb: Callback para recepção de dados
 * @param connectionCb: Callback para indicativo de conexao de client
 * @return Codigo de erro
 */
int TCPConnect(bool serverMode,_sSocket_t *socket, char *ip, uint16_t port,
CallbackReceiverTcp_t receiveCb, CallbackConnection_t connectionCb);
//***************************************************************************
/**
 * @brief Desconexao
 *
 * @param socket - Handle do socket
 * @return Codigo de erro
 */
int TCPDisconnect(_sSocket_t socket);
//***************************************************************************
/**
 * @brief Retornar o estado de conexão.
 * @param socket: Handle da conexão
 * @return true = conectado / false = sem conexão.
 */
bool TCPIsConnected (_sSocket_t  socket);
//***************************************************************************
/**
 * @brief Envio de dados
 *
 * @param socket - Handle do socket
 * @param buffer - Ponteiro do buffer de envio de dados
 * @param len - Tamanho dos dados de envio
 * @return Codigo de erro
 */
int TCPSendData(_sSocket_t socket, char *buffer, uint16_t len);

#endif /* TCP_H_ */
