/*
 * Pipe  -  Shujun. 2015
 *
 * Data can be transfered via pipe between different subsystems (or threads).
 */
#ifndef _PIPE_H
#define _PIPE_H

#include "common.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define PIPE_DEFAULT_SYNC_TIMEOUT   0  // default sync timeout: forever

#define PIPE_ERR_OK                 0
#define PIPE_ERR_NOT_OPENED        -1
#define PIPE_ERR_ALREADY_OPENED    -2
#define PIPE_ERR_WRITE_STOPPED     -3
#define PIPE_ERR_READ_STOPPED      -4
#define PIPE_ERR_SYNC_TIMEOUT      -5
#define PIPE_ERR_INVALID           -6
#define PIPE_ERR_OUT_OF_MEMORY     -7

struct pipe_struct;
typedef void (*pipe_rw_hook_cb)(struct pipe_struct * pipe, const char * buf, int bytes);

/*
 * Mutex control for pipe_t
 */
#if 1 // Use pthread mutex
#include <pthread.h>
#define pipe_lock_define() struct { pthread_mutex_t mutex; pthread_mutexattr_t mutex_attr; } mutex
#define pipe_lock_init(x) do { \
	pthread_mutexattr_settype(&x.mutex_attr, PTHREAD_MUTEX_RECURSIVE); \
	pthread_mutex_init(&x.mutex, &x.mutex_attr); } while (0)
#define pipe_lock_destroy(x) do { pthread_mutex_destroy(&x.mutex); } while (0)
#define pipe_lock(x) do { pthread_mutex_lock(&x.mutex); } while (0)
#define pipe_unlock(x) do { pthread_mutex_unlock(&x.mutex); } while(0)
#else // Use variable, Only for RTOS or No OS
#define pipe_lock_define() int mutex
#define pipe_lock_init(x) do { x = 0; } while (0)
#define pipe_lock(x) \
		do { volatile int nr = 0; \
			__sync_synchronize(); \
			while (x == 1) { \
				if (nr++ >= 1000) break; \
				log_debug("pipe_lock() in %s(), mutex:1\n", __FUNCTION__); \
				basic_sleep(); \
			} \
			x = 1; \
		} while (0)
#define pipe_unlock(x) do { x = 0; } while (0)
#endif

struct pipe_struct {
// basic information
	u8 opened:1;
	u8 stop_read:1;
	u8 stop_write:1;
	u8 sync:1;
	u8 loop_overwrite:1;
	uint user;
	uint size;
	uint bytes;
	uint read_ptr;
	uint write_ptr;
	char * buffer;
	uint read_sync_timeout;
	uint write_sync_timeout;

// mutex
	pipe_lock_define();

// hook callbacks
	pipe_rw_hook_cb read_hook;
	pipe_rw_hook_cb write_hook;

// loading information
	long file_size;  // The max size of data loading
	long readed_bytes, written_bytes;

// speed
	long readed_bytes0, written_bytes0;
	int  read_speed, write_speed;
	long last_read_time;
	long last_write_time;

// private params, used for caller
	void * priv_param;
};
typedef struct pipe_struct pipe_t;


/*
 * Open a pipe
 * @p            the pipe
 * @size         the max loading the pipe can hold.
 * @sync         1 = the pipe can be blocked if needed, 0 = don't block in any time.
 * @return val:  error code, 0 = open success
 */
int pipe_open(pipe_t * p, int size, int sync);

/*
 * Close a pipe
 * @p            the pipe
 */
void pipe_close(pipe_t * p);

/*
 * Init a pipe
 * @p            the pipe
 */
void pipe_init(pipe_t * p);

/*
 * Clone a pipe
 * @to    the target pipe which will get the data
 *        must be newly created pipe!
 * @from  the data from
 * Note: the @from and @to must have the same 'size'
 */
void pipe_clone(pipe_t * from, pipe_t * to);

/*
 * Read data from pipe (Just for peek, no changes to the pipe)
 * @p            the pipe
 * @buf          the destination buffer
 * @size         the size of @buf
 * @least        at least the number of bytes should be readed, 0 = no limit.
 * @bunch        the data should be readed in unit of 'bunch', 0 = 1 byte (as 1).
 * @return val:  bytes of readed.
 */
int pipe_peek(pipe_t * p, char * buf, unsigned int size, unsigned int least, unsigned int bunch);

/*
 * Read data from pipe
 * @p            the pipe
 * @buf          the destination buffer
 * @size         the size of @buf
 * @least        at least the number of bytes should be readed, 0 = no limit.
 * @bunch        the data should be readed in unit of 'bunch', 0 = 1 byte (as 1).
 * @return val:  bytes of readed.
 */
int pipe_read(pipe_t * p, char * buf, unsigned int size, unsigned int least, unsigned int bunch);

/*
 * Write data to pipe
 * @p            the pipe
 * @buf          the data buffer
 * @bytes        the bytes of @buf
 * @return val:  bytes of written
 */
int pipe_write(pipe_t * p, const char * buf, unsigned int bytes);

/*
 * Get free space of pipe
 * @p            the pipe
 * @return val:  free space of the pipe.
 */
int pipe_get_space(pipe_t * p);

/*
 * Get max size of pipe (max loading)
 * @p            the pipe
 * @return val:  max size.
 */
int pipe_get_size(pipe_t * p);

/*
 * Get current loading of pipe (in byte)
 * @p            the pipe
 * @return val:  bytes number the pipe holds.
 */
int pipe_get_bytes(pipe_t * p);

/*
 * Tell the pipe no read any more
 * @p            the pipe
 */
void pipe_stop_read(pipe_t * p);

/*
 * Tell the pipe no write any more
 * @p            the pipe
 */
void pipe_stop_write(pipe_t * p);

/*
 * Set a parameter to the pipe
 * @p            the pipe
 * @priv         private parameter
 */
void pipe_set_priv(pipe_t * p, void * priv);

/*
 * Get the parameter of the pipe
 * @p            the pipe
 * @return       the private parameter
 */
void * pipe_get_priv(pipe_t * p);

/*
 * Set the file size (max data loading) to the pipe
 * @p            the pipe
 * @file_size    file size
 */
void pipe_set_file_size(pipe_t * p, long file_size);

/*
 * Get the file size (max data loading) of the pipe
 * @p              the pipe
 * @readed_bytes   return total readed bytes
 * @written_bytes  return total written bytes
 * @return         file size
 */
long pipe_get_file_size(pipe_t * p, long * readed_bytes, long * written_bytes);

/*
 * Get the read speed of the pipe
 * @p            the pipe
 * @return       read speed
 */
int pipe_get_read_speed(pipe_t * p);

/*
 * Get the write speed of the pipe
 * @p            the pipe
 * @return       write speed
 */
int pipe_get_write_speed(pipe_t * p);

/*
 * Get the speed of the pipe
 * @p            the pipe
 * @return       data moving speed
 */
int pipe_get_speed(pipe_t * p);

/*
 * Set timeout for sync peek/read/write
 * @p              the pipe
 * @read_timeout   timeout value for read/peek, in millisecond, 0 = forever
 * @write_timeout  timeout value for write, in millisecond, 0 = forever
 * @return val:    error code.
 */
int pipe_set_sync_timeout(pipe_t * p, int read_timeout, int write_timeout);

/*
 * Get timeout value for sync peek/read/write
 * @p              the pipe
 * @read_timeout   timeout value for read/peek, in millisecond, 0 = forever
 * @write_timeout  timeout value for write, in millisecond, 0 = forever
 * @return val:    error code.
 */
int pipe_get_sync_timeout(pipe_t * p, int * read_timeout, int * write_timeout);

/*
 * Set hook for read
 * @p              the pipe
 * @read_hook      the hook function
 * @return val:    error code.
 */
int pipe_set_read_hook(pipe_t * p, pipe_rw_hook_cb read_hook);

/*
 * Set hook for write
 * @p              the pipe
 * @write_hook     the hook function
 * @return val:    error code.
 */
int pipe_set_write_hook(pipe_t * p, pipe_rw_hook_cb write_hook);

/*
 * Set loop overwrite feature for write
 * @p               the pipe
 * @loop_overwrite  loop overwrite
 * @return val:     error code.
 *
 * Note: loop overwrite works for NOT-Sync pipe.
 */
int pipe_set_loop_overwrite(pipe_t * p, int loop_overwrite);


#ifdef __cplusplus
}
#endif

#endif /* _PIPE_H */
