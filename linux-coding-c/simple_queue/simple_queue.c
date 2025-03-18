/**
 * @file    simple_queue.c
 * @brief   Completement of simple_queue.h
 * @author
 * @version 1.0.0
 * @date
 * @copyright
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "simple_queue.h"
#include "log.h"

// pthread_cond_t msg_cond = PTHREAD_COND_INITIALIZER;
simple_queue *create_simple_queue(const char *queue_name, int queue_length, int queue_type)
{
    simple_queue *this = NULL;
    if (NULL == queue_name || 0 == queue_length)
    {
        log_warn("param is error\n");
        return NULL;
    }
    this = (simple_queue *)malloc(sizeof(simple_queue) + queue_length * sizeof(queue_buf));
    if (NULL != this)
    {
        this->front = 0;
        this->rear = 0;
        this->length = queue_length;
        this->queue_type = queue_type;
        if (0 != pthread_mutex_init(&(this->data_mutex), NULL))
        {
            log_err("pthread_mutex_init failed!\n");
            free(this);
            this = NULL;
            return NULL;
        }
        if (0 != pthread_cond_init(&(this->msg_cond), NULL))
        {
            log_err("pthread_cond_init failed!\n");
            free(this);
            this = NULL;
            return NULL;
        }
        strcpy(this->queue_name, queue_name);
    }
    else
    {
        log_err("malloc is failed!\n");
        return NULL;
    }
    return this;
}

queue_status is_full_queue(simple_queue *p_queue)
{
    queue_status ret = QUEUE_IS_NORMAL;
    do
    {
        if (NULL == p_queue)
        {
            log_warn("param is error\n");
            ret = QUEUE_NO_EXIST;
            break;
        }
        if (p_queue->front == ((p_queue->rear + 1) % (p_queue->length)))
        {
            log_warn("queue is full\n");
            ret = QUEUE_IS_FULL;
            break;
        }
    } while (0);
    return ret;
}

queue_status is_empty_queue(simple_queue *p_queue)
{
    queue_status ret = QUEUE_IS_NORMAL;
    do
    {
        if (NULL == p_queue)
        {
            log_warn("param is error\n");
            ret = QUEUE_NO_EXIST;
            break;
        }
        if (p_queue->front == p_queue->rear)
        {
            log_warn("queue is empty\n");
            ret = QUEUE_IS_EMPTY;
            break;
        }
    } while (0);
    return ret;
}

cntl_queue_ret push_simple_queue(simple_queue *p_queue, queue_buf *pdata)
{
    int w_cursor = 0;
    if (NULL == p_queue || NULL == pdata)
    {
        log_warn("param is error\n");
        return CNTL_QUEUE_PARAM_ERROR;
    }
    pthread_mutex_lock(&(p_queue->data_mutex));
    w_cursor = (p_queue->rear + 1) % p_queue->length;
    if (w_cursor == p_queue->front)        ///< queue is full, need to block
    {
        log_never("queue is full, wait for a moment\n");
        pthread_cond_wait(&(p_queue->msg_cond), &(p_queue->data_mutex));
        if (w_cursor == p_queue->front)
        {
            log_warn("queue is still full now\n");
            pthread_mutex_unlock(&(p_queue->data_mutex));
            return CNTL_QUEUE_FAIL;
        }
    }
    p_queue->data[p_queue->rear] = *pdata;
    p_queue->rear = w_cursor;
    pthread_mutex_unlock(&(p_queue->data_mutex));
    pthread_cond_signal(&(p_queue->msg_cond));
    return CNTL_QUEUE_SUCCESS;
}

cntl_queue_ret pop_simple_queue(simple_queue *p_queue, queue_buf *pdata)
{
    if (NULL == p_queue)
    {
        log_warn("param is error\n");
        return CNTL_QUEUE_PARAM_ERROR;
    }
    pthread_mutex_lock(&(p_queue->data_mutex));
    if (p_queue->front == p_queue->rear)        ///< queue is empty, need to block
    {
        log_never("queue is empty, wait for a moment\n");
        pthread_cond_wait(&(p_queue->msg_cond), &(p_queue->data_mutex));
        if (p_queue->front == p_queue->rear)
        {
            log_warn("queue is still empty now\n");
            pthread_mutex_unlock(&(p_queue->data_mutex));
            return CNTL_QUEUE_FAIL;
        }
    }
    *pdata = p_queue->data[p_queue->front];
    p_queue->front = (p_queue->front + 1) % p_queue->length;
    pthread_mutex_unlock(&(p_queue->data_mutex));
    pthread_cond_signal(&(p_queue->msg_cond));
    return CNTL_QUEUE_SUCCESS;
}

cntl_queue_ret destroy_simple_queue(simple_queue *p_queue)
{
    cntl_queue_ret ret = CNTL_QUEUE_SUCCESS;
    if (NULL == p_queue)
    {
        log_warn("param is error\n");
        ret = CNTL_QUEUE_PARAM_ERROR;
    }
    else
    {
        // 通知所有等待的线程队列将要被销毁
        pthread_cond_broadcast(&(p_queue->msg_cond));
        // 销毁互斥锁和条件变量
        pthread_mutex_destroy(&(p_queue->data_mutex));
        pthread_cond_destroy(&(p_queue->msg_cond));
        while (p_queue->front != p_queue->rear) // 删除队列中残留的消息
        {
            free_queue_buf(&p_queue->data[p_queue->front]);
            p_queue->front = (p_queue->front + 1) % p_queue->length;
        }
        free(p_queue);
    }
    return ret;
}

void free_queue_buf(queue_buf *queue_msg)
{
    if (queue_msg->msg_buf != NULL)
    {
        free(queue_msg->msg_buf);
        queue_msg->msg_buf = NULL;
    }
}
