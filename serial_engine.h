#ifndef SERIAL_ENGINE_H
# define SERIAL_ENGINE_H

# include <pthread.h>
# include <stdint.h>

# define RB_SIZE 4096
# define PACKET_HEADER 0x42
# define PACKET_SIZE 23
# define SENSOR_COUNT 3
# define DEFAULT_BAUD B115200

/*
** Ring Buffer
** Lock-free safe for single producer / single consumer.
** Mutex added for multi-thread safety on rb_available / rb_peek.
*/

typedef struct s_ring_buffer
{
	uint8_t			buf[RB_SIZE];
	volatile int	head;
	volatile int	tail;
	pthread_mutex_t	mutex;
}	t_ring_buffer;

/*
** Car State
** Updated atomically by the receiver thread.
** Read by the Python side via serial_get_state().
*/

typedef struct s_car_state
{
	float			x;
	float			y;
	float			speed;
	float			angle;
	uint16_t		sensor_dist[SENSOR_COUNT];
	uint64_t		timestamp;
	pthread_mutex_t	mutex;
}	t_car_state;

/*
** Serial Context
** Holds everything needed for one serial connection.
*/

typedef struct s_serial_ctx
{
	int				fd;
	volatile int	running;
	pthread_t		thread;
	t_ring_buffer	rb;
	t_car_state		state;
}	t_serial_ctx;

/*
** Public API — exported to Python via ctypes
*/

t_serial_ctx	*serial_open(const char *device, int baud);
int				serial_start_listener(t_serial_ctx *ctx);
int				serial_get_state(t_serial_ctx *ctx, t_car_state *out);
int				serial_send_command(t_serial_ctx *ctx, const uint8_t *data, int len);
void			serial_close(t_serial_ctx *ctx);

/*
** Internal — ring buffer helpers
*/

void			rb_init(t_ring_buffer *rb);
int				rb_write(t_ring_buffer *rb, const uint8_t *data, int len);
int				rb_read(t_ring_buffer *rb, uint8_t *dest, int len);
int				rb_available(t_ring_buffer *rb);
int				rb_peek(t_ring_buffer *rb, uint8_t *dest, int len);
void			rb_discard(t_ring_buffer *rb, int len);

#endif
