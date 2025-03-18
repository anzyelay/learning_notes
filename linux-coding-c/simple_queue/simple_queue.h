/**
 * @file    simple_queue.h
 * @brief   Simple queue with block function
 * @copyright
 */
#ifndef _SIMPLE_QUEUE_H__
#define _SIMPLE_QUEUE_H__

#define MQ_LENGTH_MAX 128 // queue size

#include <pthread.h>

typedef struct _simple_queue_buf
{
    int msg_len;
    void *msg_buf;
} queue_buf;

typedef struct _simple_queue
{
    int front;
    int rear;
    int length;
    int queue_type;
    pthread_mutex_t data_mutex;
    pthread_cond_t msg_cond;
    char queue_name[15];
    queue_buf data[];
} simple_queue;

typedef enum _queue_type
{
    QUEUE_BLOCK = 0,
    QUEUE_NO_BLOCK,
} queue_type;

/**
 * @enum        queue_status
 * @brief       Queue status
 */
typedef enum _queue_status
{
    QUEUE_IS_NORMAL = 0,
    QUEUE_NO_EXIST,
    QUEUE_IS_FULL,
    QUEUE_IS_EMPTY,
} queue_status;

typedef enum _cntl_queue_ret
{
    CNTL_QUEUE_SUCCESS = 0,
    CNTL_QUEUE_FAIL,
    CNTL_QUEUE_TIMEOUT,
    CNTL_QUEUE_PARAM_ERROR,
} cntl_queue_ret;

/**
 * @brief   Create a queue object
 *
 * @param   queue_name  Name of queue
 * @param   queue_length Maximum length of queue
 * @param   queue_type  Type of queue
 * @return  simple_queue* pointer for queue object
 */
simple_queue *create_simple_queue(const char *queue_name, int queue_length, int queue_type);

/**
 * @brief Check that the queue is full or not
 *
 * @param p_queue       Queue object
 * @return enum         Queue status
 *   @retval 0          Queue is normal
 *   @retval 1          Queue dosen't exist
 *   @retval 2          Queue is full
 *   @retval 3          Queue is empty
 */
queue_status is_full_queue(simple_queue *p_queue);

/**
 * @brief Check that the queue is empty or not
 *
 * @param p_queue       Queue object
 * @return enum         Queue status
 *   @retval 0          Queue is normal
 *   @retval 1          Queue dosen't exist
 *   @retval 2          Queue is full
 *   @retval 3          Queue is empty
 */
queue_status is_empty_queue(simple_queue *p_queue);

/**
 * @brief Enqueue a queue element
 *
 * @param p_queue       Queue object
 * @param data          Element data
 * @return enum         Reusult of controlling queue
 *   @retval 0          Success
 *   @retval 1          Fail
 *   @retval 2          Timeout
 *   @retval 3          Parameter error
 */
cntl_queue_ret push_simple_queue(simple_queue *p_queue, queue_buf *pdata);

/**
 * @brief Dequeue a queue element
 *
 * @param p_queue       Queue object
 * @param data          Element data
 * @return enum         Reusult of controlling queue
 *   @retval 0          Success
 *   @retval 1          Fail
 *   @retval 2          Timeout
 *   @retval 3          Parameter error
 */
cntl_queue_ret pop_simple_queue(simple_queue *p_queue, queue_buf *pdata);

/**
 * @brief Destroy the queue object
 *
 * @param p_queue       Queue object
 * @return enum         Reusult of controlling queue
 *   @retval 0          Success
 *   @retval 1          Fail
 *   @retval 2          Timeout
 *   @retval 3          Parameter error
 */
cntl_queue_ret destroy_simple_queue(simple_queue *p_queue);

/**
 * @brief   Release the queue element's space
 *
 * @param   queue_msg   Queue element
 */
void free_queue_buf(queue_buf *queue_msg);

#endif
