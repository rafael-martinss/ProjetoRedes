/**
 ******************************************************************************
 * @file    thread_wrapper.c
 * @author  Rafael Martins
 *****************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include "thread_wrapper.h"

int threadCreate(sThread_t *threadHandle, char* name, _task_function_t invokable, void* param)
{
	pthread_attr_t thrAttb;
	pthread_t handle;
	int ret;

	memset(threadHandle, 0, sizeof(*threadHandle));
	memset(&thrAttb, 0, sizeof(thrAttb));
	ret = pthread_attr_init(&thrAttb);
	if(ret)
	{
		goto error;
	}

	ret = pthread_create(&handle, &thrAttb, invokable, param);
	pthread_attr_destroy(&thrAttb);
	if(ret)
	{
		goto error;
	}
	threadHandle->handle = handle;
	memcpy(threadHandle->name, name, (strlen(name) + 1 > THREAD_MAX_NAME_SZ)? THREAD_MAX_NAME_SZ : strlen(name));
	return 0;

error:
	printf("Thread %s create failure %d - %s \n", name, errno, strerror(errno));
	return ret;
}

//***************************************************************************
int threadExit(void)
{
	pthread_exit(NULL);
	return 0;
}
