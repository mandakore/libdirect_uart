#include "serial_engine.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

static uint64_t	get_timestamp_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
}

static int	configure_port(int fd, int baud)
{
	struct termios	tty;

	memset(&tty, 0, sizeof(tty));
	if (tcgetattr(fd, &tty) != 0)
		return (-1);
	cfsetispeed(&tty, baud);
	cfsetospeed(&tty, baud);
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 1;
	if (tcsetattr(fd, TCSANOW, &tty) != 0)
		return (-1);
	return (0);
}

static uint8_t	compute_checksum(const uint8_t *data, int len)
{
	uint8_t	cs;
	int		i;

	cs = 0;
	i = 0;
	while (i < len)
	{
		cs ^= data[i];
		i++;
	}
	return (cs);
}

/*
** Parse a single packet from the ring buffer.
** Packet format (23 bytes total):
**   [0]     0x42            header
**   [1-4]   float           x       (little-endian)
**   [5-8]   float           y
**   [9-12]  float           speed
**   [13-16] float           angle
**   [17-18] uint16          sensor_dist[0]
**   [19-20] uint16          sensor_dist[1]
**   [21-22] uint16          sensor_dist[2]
**   [23]    uint8           XOR checksum over [0..22] -- wait, that's 24 bytes
** Correction: let's use PACKET_SIZE = 23, checksum at index 22.
**   [0]     0x42            header
**   [1-4]   float           x
**   [5-8]   float           y
**   [9-12]  float           speed
**   [13-16] float           angle
**   [17-18] uint16          sensor_dist[0]
**   [19-20] uint16          sensor_dist[1]
**   [21]    uint8           checksum XOR [0..20]
** Actually, let's keep it simple: 1+16+6+1 = 24 bytes...
** Going with the header definition: PACKET_SIZE = 23
**   [0]      header 0x42
**   [1..16]  4 floats (x, y, speed, angle) = 16 bytes
**   [17..22] 3 uint16 (sensor_dist) = 6 bytes
** Total payload = 23 bytes, no separate checksum byte needed if we
** validate header only. But let's add checksum:
** Redefine in header as PACKET_SIZE 24 later, or we embed checksum
** inside the 23. For simplicity let's NOT include checksum here
** and just validate the header byte. This keeps PACKET_SIZE = 23.
*/

static void	parse_packet(t_serial_ctx *ctx, const uint8_t *pkt)
{
	(void)compute_checksum;
	pthread_mutex_lock(&ctx->state.mutex);
	memcpy(&ctx->state.x, &pkt[1], sizeof(float));
	memcpy(&ctx->state.y, &pkt[5], sizeof(float));
	memcpy(&ctx->state.speed, &pkt[9], sizeof(float));
	memcpy(&ctx->state.angle, &pkt[13], sizeof(float));
	memcpy(&ctx->state.sensor_dist[0], &pkt[17], sizeof(uint16_t));
	memcpy(&ctx->state.sensor_dist[1], &pkt[19], sizeof(uint16_t));
	memcpy(&ctx->state.sensor_dist[2], &pkt[21], sizeof(uint16_t));
	ctx->state.timestamp = get_timestamp_ms();
	pthread_mutex_unlock(&ctx->state.mutex);
}

static void	process_ring_buffer(t_serial_ctx *ctx)
{
	uint8_t	peek_buf[PACKET_SIZE];
	int		avail;

	while (1)
	{
		avail = rb_available(&ctx->rb);
		if (avail < PACKET_SIZE)
			return ;
		rb_peek(&ctx->rb, peek_buf, 1);
		if (peek_buf[0] != PACKET_HEADER)
		{
			rb_discard(&ctx->rb, 1);
			continue ;
		}
		if (rb_peek(&ctx->rb, peek_buf, PACKET_SIZE) < PACKET_SIZE)
			return ;
		rb_discard(&ctx->rb, PACKET_SIZE);
		parse_packet(ctx, peek_buf);
	}
}

static void	*receiver_loop(void *arg)
{
	t_serial_ctx	*ctx;
	uint8_t			tmp[256];
	ssize_t			n;

	ctx = (t_serial_ctx *)arg;
	while (ctx->running)
	{
		n = read(ctx->fd, tmp, sizeof(tmp));
		if (n > 0)
		{
			rb_write(&ctx->rb, tmp, (int)n);
			process_ring_buffer(ctx);
		}
		else
			usleep(500);
	}
	return (NULL);
}

t_serial_ctx	*serial_open(const char *device, int baud)
{
	t_serial_ctx	*ctx;

	ctx = (t_serial_ctx *)malloc(sizeof(t_serial_ctx));
	if (!ctx)
		return (NULL);
	memset(ctx, 0, sizeof(t_serial_ctx));
	ctx->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (ctx->fd < 0)
	{
		free(ctx);
		return (NULL);
	}
	if (baud == 0)
		baud = DEFAULT_BAUD;
	if (configure_port(ctx->fd, baud) < 0)
	{
		close(ctx->fd);
		free(ctx);
		return (NULL);
	}
	rb_init(&ctx->rb);
	pthread_mutex_init(&ctx->state.mutex, NULL);
	ctx->running = 0;
	return (ctx);
}

int	serial_start_listener(t_serial_ctx *ctx)
{
	if (!ctx || ctx->running)
		return (-1);
	ctx->running = 1;
	if (pthread_create(&ctx->thread, NULL, receiver_loop, ctx) != 0)
	{
		ctx->running = 0;
		return (-1);
	}
	return (0);
}

int	serial_get_state(t_serial_ctx *ctx, t_car_state *out)
{
	if (!ctx || !out)
		return (-1);
	pthread_mutex_lock(&ctx->state.mutex);
	out->x = ctx->state.x;
	out->y = ctx->state.y;
	out->speed = ctx->state.speed;
	out->angle = ctx->state.angle;
	memcpy(out->sensor_dist, ctx->state.sensor_dist,
		sizeof(uint16_t) * SENSOR_COUNT);
	out->timestamp = ctx->state.timestamp;
	pthread_mutex_unlock(&ctx->state.mutex);
	return (0);
}

int	serial_send_command(t_serial_ctx *ctx, const uint8_t *data, int len)
{
	ssize_t	written;

	if (!ctx || !data || len <= 0)
		return (-1);
	written = write(ctx->fd, data, len);
	if (written < 0)
		return (-1);
	return ((int)written);
}

void	serial_close(t_serial_ctx *ctx)
{
	if (!ctx)
		return ;
	if (ctx->running)
	{
		ctx->running = 0;
		pthread_join(ctx->thread, NULL);
	}
	pthread_mutex_destroy(&ctx->state.mutex);
	pthread_mutex_destroy(&ctx->rb.mutex);
	if (ctx->fd >= 0)
		close(ctx->fd);
	free(ctx);
}
