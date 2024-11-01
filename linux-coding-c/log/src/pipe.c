/*
 * Pipe  -  Shujun. 2015
 *
 * Data can be transfered via pipe between different subsystems (or threads).
 */
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "common.h"
#include "pipe.h"

#define basic_sleep()  usleep(1000 * 10)
#define configTICK_RATE_HZ  1000

/*
 * Open a pipe
 * @p            the pipe
 * @size         the max loading the pipe can hold.
 * @sync         1 = the pipe can be blocked if needed, 0 = don't block in any time.
 * @return val:  error code, 0 = open success.
 */
int pipe_open(pipe_t * p, int size, int sync)
{
	if (!p || p->opened)
		return PIPE_ERR_ALREADY_OPENED;

	pipe_init(p);
	p->stop_read = 0;
	p->stop_write = 0;
	p->sync = !!sync;
	p->loop_overwrite = 0;
	p->read_sync_timeout = PIPE_DEFAULT_SYNC_TIMEOUT;
	p->write_sync_timeout = PIPE_DEFAULT_SYNC_TIMEOUT;
	p->read_hook = NULL;
	p->write_hook = NULL;
	p->user = 0;
	p->size = size;
	p->bytes = 0;
	p->read_ptr = 0;
	p->write_ptr = 0;
	p->priv_param = NULL;
	p->file_size = 0;
	p->readed_bytes = 0;
	p->written_bytes = 0;
	p->read_speed = 0;
	p->write_speed = 0;
	p->last_read_time = get_time_ms();
	p->last_write_time = get_time_ms();
	pipe_lock_init(p->mutex);
	p->opened = 1;
	p->buffer = (char *)malloc(p->size);
	if (p->buffer)
		return PIPE_ERR_OK;

	pipe_init(p);
	p->opened = 0;
	return PIPE_ERR_OUT_OF_MEMORY;
}

/*
 * Close a pipe
 * @p            the pipe
 */
void pipe_close(pipe_t * p)
{
	if (!p || !p->opened)
		return;

	pipe_stop_read(p);
	pipe_stop_write(p);
	while (p->user > 0) {
		basic_sleep();
	}

	p->opened = 0; // set close first, then free buffer
	if (p->buffer)
		free(p->buffer);
	pipe_init(p);
}

/*
 * Clone a pipe
 * @to    the target pipe which will get the data
 *        must be newly created pipe!
 * @from  the data from
 * Note: the @from and @to must have the same 'size'
 */
void pipe_clone(pipe_t * from, pipe_t * to)
{
	char * buffer = to->buffer;

	if (pipe_get_size(from) != pipe_get_size(to))
		return;

	pipe_lock(from->mutex);
	*to = *from;
	to->buffer = buffer;
	to->priv_param = NULL;
	pipe_lock_init(to->mutex);
	memcpy(buffer, from->buffer, from->size);
	pipe_unlock(from->mutex);
}

/*
 * Init a pipe
 * @p            the pipe
 */
void pipe_init(pipe_t * p)
{
	if (!p)
		return;
	memset(p, 0, sizeof(*p));
}

/*
 * Read data from pipe (Just for peek, no changes to the pipe)
 * @p            the pipe
 * @buf          the destination buffer
 * @size         the size of @buf
 * @least        at least the number of bytes should be readed, 0 = no limit.
 * @bunch        the data should be readed in unit of 'bunch', 0 = 1 byte (as 1).
 * @return val:  bytes of readed.
 */
int pipe_peek(pipe_t * p, char * buf, unsigned int size, unsigned int least, unsigned int bunch)
{
	long now, start = get_time_ms();

	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;
	pipe_lock(p->mutex);
	p->user++;

	// No data available
	while (p->bytes == 0) {
		if (p->stop_write) {
			p->user--;
			pipe_unlock(p->mutex);
			return PIPE_ERR_WRITE_STOPPED; // No data any more!
		}
		if (p->sync) {
			basic_sleep();
			if (p->read_sync_timeout > 0) {
				now = get_time_ms();
				if (now - start >= p->read_sync_timeout) {
					p->user--;
					pipe_unlock(p->mutex);
					return PIPE_ERR_SYNC_TIMEOUT;
				}
			}
			continue;
		} else {
			p->user--;
			pipe_unlock(p->mutex);
			return 0;
		}
	}

	// least & bunch
	if (bunch < 1)
		bunch = 1;
	least -= (least % bunch);
	if (least < bunch)
		least = bunch;
	while (p->bytes < least) {
		if (p->stop_write)
			break;
		basic_sleep();
	}

	// Has data
	if (size > p->bytes)
		size = p->bytes;
	size -= (size % bunch);
	if ((p->size - p->read_ptr) >= size) {
		memcpy(buf, p->buffer + p->read_ptr, size);
	} else { // The buffer is looped back
		int size1 = (p->size - p->read_ptr);
		memcpy(buf, p->buffer + p->read_ptr, size1);
		memcpy(buf + size1, p->buffer, size - size1);
	}

	p->user--;
	pipe_unlock(p->mutex);

	return size;
}

/*
 * Read data from pipe
 * @p            the pipe
 * @buf          the destination buffer
 * @size         the size of @buf
 * @least        at least the number of bytes should be readed, 0 = no limit.
 * @bunch        the data should be readed in unit of 'bunch', 0 = 1 byte (as 1).
 * @return val:  bytes of readed.
 */
int pipe_read(pipe_t * p, char * buf, unsigned int size, unsigned int least, unsigned int bunch)
{
	long now, start = get_time_ms();

	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;
	pipe_lock(p->mutex);
	p->user++;

	// No data available
	while (p->bytes == 0) {
		if (p->stop_write) {
			p->user--;
			pipe_unlock(p->mutex);
			return PIPE_ERR_WRITE_STOPPED; // No data any more!
		}
		if (p->sync) {
			basic_sleep();
			if (p->read_sync_timeout > 0) {
				now = get_time_ms();
				if (now - start >= p->read_sync_timeout) {
					p->user--;
					pipe_unlock(p->mutex);
					return PIPE_ERR_SYNC_TIMEOUT;
				}
			}
			continue;
		} else {
			p->user--;
			pipe_unlock(p->mutex);
			return 0;
		}
	}

	// least & bunch
	if (bunch < 1)
		bunch = 1;
	least -= (least % bunch);
	if (least < bunch)
		least = bunch;
	while (p->bytes < least) {
		if (p->stop_write)
			break;
		basic_sleep();
	}

	// Has data
	if (size > p->bytes)
		size = p->bytes;
	size -= (size % bunch);
	if ((p->size - p->read_ptr) >= size) {
		memcpy(buf, p->buffer + p->read_ptr, size);
		p->read_ptr += size;
		if (p->read_ptr >= p->size)
			p->read_ptr = 0;
	} else { // The buffer is looped back
		int size1 = (p->size - p->read_ptr);
		memcpy(buf, p->buffer + p->read_ptr, size1);
		memcpy(buf + size1, p->buffer, size - size1);
		p->read_ptr = size - size1;
	}
	p->bytes -= size;

	p->readed_bytes += size;
	p->readed_bytes0 += size;
	if (get_time_ms() - p->last_read_time >= configTICK_RATE_HZ) {
		p->read_speed = p->readed_bytes0 * configTICK_RATE_HZ / (get_time_ms() - p->last_read_time);
		p->readed_bytes0 = 0;
		p->last_read_time = get_time_ms();
	}

	if (p->read_hook)
		p->read_hook(p, buf, size);

	p->user--;
	pipe_unlock(p->mutex);

	return size;
}

/*
 * Write data to pipe
 * @p            the pipe
 * @buf          the data buffer
 * @bytes        the bytes of @buf
 * @return val:  bytes of written.
 */
int pipe_write(pipe_t * p, const char * buf, unsigned int bytes)
{
	long now, start = get_time_ms();
	unsigned int overwrite;

	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;
	pipe_lock(p->mutex);
	p->user++;

	if (p->stop_read) {
		p->user--;
		pipe_unlock(p->mutex);
		return PIPE_ERR_READ_STOPPED;
	}

	if (bytes == 0) {
		p->user--;
		pipe_unlock(p->mutex);
		return 0;
	}

	// No space available
	while (p->bytes == p->size) {
		if (p->stop_read) {
			p->user--;
			pipe_unlock(p->mutex);
			return PIPE_ERR_READ_STOPPED; // No reader!
		}
		if (p->sync) {
			basic_sleep();
			if (p->write_sync_timeout > 0) {
				now = get_time_ms();
				if (now - start >= p->write_sync_timeout) {
					p->user--;
					pipe_unlock(p->mutex);
					return PIPE_ERR_SYNC_TIMEOUT;
				}
			}
			continue;
		} else if (p->loop_overwrite) {
			break;
		} else {
			p->user--;
			pipe_unlock(p->mutex);
			return 0;
		}
	}

	// Loop overwrite, to keep the pipe's data up-to-date
	if (p->loop_overwrite && !p->sync) {
		if (bytes > (p->size - p->bytes)) {
			overwrite = bytes - (p->size - p->bytes);
			if (overwrite < 0)
				overwrite = 0;
			if (overwrite > bytes)
				overwrite = bytes;
			if (overwrite > p->size)
				overwrite = p->size;

			p->bytes -= overwrite;
			p->read_ptr += overwrite;
			if (p->read_ptr >= p->size)
				p->read_ptr -= p->size;
		}
	}

	// Has space
	if (bytes > (p->size - p->bytes))
		bytes = (p->size - p->bytes);

	if ((p->size - p->write_ptr) >= bytes) {
		memcpy(p->buffer + p->write_ptr, buf, bytes);
		p->write_ptr += bytes;
		if (p->write_ptr >= p->size)
			p->write_ptr = 0;
	} else { // The buffer is looped back
		int size1 = (p->size - p->write_ptr);
		memcpy(p->buffer + p->write_ptr, buf, size1);
		memcpy(p->buffer, buf + size1, bytes - size1);
		p->write_ptr = bytes - size1;
	}

	p->bytes += bytes;
	p->written_bytes += bytes;
	if (p->written_bytes > p->file_size)
		p->file_size = p->written_bytes;

	p->written_bytes0 += bytes;
	if (get_time_ms() - p->last_write_time >= configTICK_RATE_HZ) {
		p->write_speed = p->written_bytes0 * configTICK_RATE_HZ / (get_time_ms() - p->last_write_time);
		p->written_bytes0 = 0;
		p->last_write_time = get_time_ms();
	}

	if (p->write_hook)
		p->write_hook(p, buf, bytes);

	p->user--;
	pipe_unlock(p->mutex);

	return bytes;
}

/*
 * Get free space of pipe
 * @p            the pipe
 * @return val:  free space of the pipe.
 */
int pipe_get_space(pipe_t * p)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;
	return (p->size - p->bytes);
}

/*
 * Get max size of pipe (max loading)
 * @p            the pipe
 * @return val:  max size.
 */
int pipe_get_size(pipe_t * p)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;
	return (p->size);
}

/*
 * Get current loading of pipe (in byte)
 * @p            the pipe
 * @return val:  bytes number the pipe holds.
 */
int pipe_get_bytes(pipe_t * p)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;
	if (p->stop_write)
		return PIPE_ERR_WRITE_STOPPED;
	return (p->bytes);
}

/*
 * Tell the pipe no read any more
 * @p            the pipe
 */
void pipe_stop_read(pipe_t * p)
{
	if (!p || !p->opened)
		return;
	p->stop_read = 1;
}

/*
 * Tell the pipe no write any more
 * @p            the pipe
 */
void pipe_stop_write(pipe_t * p)
{
	if (!p || !p->opened)
		return;
	p->stop_write = 1;
}

/*
 * Set a parameter to the pipe
 * @p            the pipe
 * @priv         private parameter
 */
void pipe_set_priv(pipe_t * p, void * priv)
{
	if (!p || !p->opened)
		return;
	p->priv_param = priv;
}

/*
 * Get the parameter of the pipe
 * @p            the pipe
 * @return       the private parameter
 */
void * pipe_get_priv(pipe_t * p)
{
	if (!p || !p->opened)
		return NULL;
	return p->priv_param;
}

/*
 * Set the file size (max data loading) to the pipe
 * @p            the pipe
 * @file_size    file size
 */
void pipe_set_file_size(pipe_t * p, long file_size)
{
	if (!p || !p->opened)
		return;
	p->file_size = file_size;
}

/*
 * Get the file size (max data loading) of the pipe
 * @p              the pipe
 * @readed_bytes   return total readed bytes
 * @written_bytes  return total written bytes
 * @return         file size
 */
long pipe_get_file_size(pipe_t * p, long * readed_bytes, long * written_bytes)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	if (readed_bytes)
		*readed_bytes = p->readed_bytes;
	if (written_bytes)
		*written_bytes = p->written_bytes;

	return p->file_size;
}

/*
 * Get the read speed of the pipe
 * @p            the pipe
 * @return       read speed
 */
int pipe_get_read_speed(pipe_t * p)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	return p->read_speed;
}

/*
 * Get the write speed of the pipe
 * @p            the pipe
 * @return       write speed
 */
int pipe_get_write_speed(pipe_t * p)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	return p->write_speed;
}

/*
 * Get the speed of the pipe
 * @p            the pipe
 * @return       data moving speed
 */
int pipe_get_speed(pipe_t * p)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	return (p->read_speed <= p->write_speed) ? p->read_speed : p->write_speed;
}

/*
 * Set timeout for sync peek/read/write
 * @p              the pipe
 * @read_timeout   timeout value for read/peek, in millisecond, 0 = forever
 * @write_timeout  timeout value for write, in millisecond, 0 = forever
 * @return val:    error code.
 */
int pipe_set_sync_timeout(pipe_t * p, int read_timeout, int write_timeout)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;
	if (read_timeout < 0 && write_timeout < 0)
		return PIPE_ERR_INVALID;

	if (read_timeout >= 0)
		p->read_sync_timeout = (uint)read_timeout;
	if (write_timeout >= 0)
		p->write_sync_timeout = (uint)write_timeout;

	return PIPE_ERR_OK;
}

/*
 * Get timeout value for sync peek/read/write
 * @p              the pipe
 * @read_timeout   timeout value for read/peek, in millisecond, 0 = forever
 * @write_timeout  timeout value for write, in millisecond, 0 = forever
 * @return val:    error code.
 */
int pipe_get_sync_timeout(pipe_t * p, int * read_timeout, int * write_timeout)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	if (read_timeout)
		*read_timeout = (int)p->read_sync_timeout;
	if (write_timeout)
		*write_timeout = (int)p->write_sync_timeout;

	return PIPE_ERR_OK;
}

/*
 * Set hook for read
 * @p              the pipe
 * @read_hook      the hook function
 * @return val:    error code.
 */
int pipe_set_read_hook(pipe_t * p, pipe_rw_hook_cb read_hook)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	p->read_hook = read_hook;

	return PIPE_ERR_OK;
}

/*
 * Set hook for write
 * @p              the pipe
 * @write_hook     the hook function
 * @return val:    error code.
 */
int pipe_set_write_hook(pipe_t * p, pipe_rw_hook_cb write_hook)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	p->write_hook = write_hook;

	return PIPE_ERR_OK;
}

/*
 * Set loop overwrite feature for write
 * @p               the pipe
 * @loop_overwrite  loop overwrite
 * @return val:     error code.
 *
 * Note: loop overwrite works for NOT-Sync pipe.
 */
int pipe_set_loop_overwrite(pipe_t * p, int loop_overwrite)
{
	if (!p || !p->opened)
		return PIPE_ERR_NOT_OPENED;

	if (p->sync)
		return PIPE_ERR_INVALID;

	p->loop_overwrite = !!loop_overwrite;

	return PIPE_ERR_OK;
}
