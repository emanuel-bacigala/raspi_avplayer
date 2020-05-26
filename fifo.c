#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fifo.h"

static int FIFO_SIZE;

static buff_element *fifo_buffer;

static int head, tail;
static int elementSize;
static pthread_mutex_t mutex;


void fifoInit(int fifoLen, int elementLen)
{
    int i;

    FIFO_SIZE = fifoLen + 1;

    elementSize = elementLen;
    fifo_buffer = (buff_element *) malloc(FIFO_SIZE * sizeof(buff_element));

    for(i = 0; i < FIFO_SIZE; i++)
    {
        fifo_buffer[i].pixBuffer = (unsigned char *)malloc(elementSize);
    }

    head = tail = 0;
}


buff_element* fifoPut(void)
{
    int dstIndex;

  pthread_mutex_lock(&mutex);

    if ( head == (( tail - 1 + FIFO_SIZE) % FIFO_SIZE) )
    {
        pthread_mutex_unlock(&mutex);
        return NULL;  // FIFO Full
    }

    dstIndex = head;
    head = (head + 1) % FIFO_SIZE;
  pthread_mutex_unlock(&mutex);
    return &fifo_buffer[dstIndex];
}


buff_element* fifoGet(void)
{
    buff_element *retPtr = NULL;

    pthread_mutex_lock(&mutex);

    if (head == tail)
    {
        pthread_mutex_unlock(&mutex);

        return NULL;  // FIFO Empty
    }

    retPtr = (buff_element*)&fifo_buffer[tail];

    tail = (tail + 1) % FIFO_SIZE;

    pthread_mutex_unlock(&mutex);

    return retPtr;
}


int fifoGetNumElements(void)
{
    int len = head - tail;

    if (len < 0)
        return FIFO_SIZE + len;

    return len;
}


void fifoRelease(void)
{
    int i;

    for(i = 0; i < FIFO_SIZE; i++)
    {
        free(fifo_buffer[i].pixBuffer);
    }

    free(fifo_buffer);
}

