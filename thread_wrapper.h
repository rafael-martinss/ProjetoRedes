/**
 ******************************************************************************
 * @file    thread_wrapper.h
 * @author  Rafael Martins
 ******************************************************************************
 */

#ifndef THREAD_WRAPPER_H_
#define THREAD_WRAPPER_H_

#include <stdint.h>
#include <pthread.h>

#define THREAD_MAX_NAME_SZ 10

typedef struct
{
	pthread_t handle;
	char name[THREAD_MAX_NAME_SZ];
}sThread_t;

typedef void* (*_task_function_t)(void *param);

/*****************************************************************************/
/**
 * @brief Criacao de thread
 *
 * @param threadHandle - Ponteiro para armazenar o handle
 * @param name - NOme da task (para colaborar na depuracao)
 * @param invokable - ponteiro para a funcao de execucao da thread
 * @param param - Parametro para a instancia da thread
 * @return codigo de erro
 */
int threadCreate(sThread_t *threadHandle, char* name, _task_function_t invokable, void* param);

//***************************************************************************
/**
 * @brief Finaliza a thread. Caso para quando a thread que se auto eliminar
 *
 * @return codigo de erro
 */
int threadExit(void);

#endif /* THREAD_WRAPPER_H_ */
