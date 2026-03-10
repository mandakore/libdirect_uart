#include "serial_engine.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int configure_port(int fd) {
  struct termios tio;

  memset(&tio, 0, sizeof(tio));
  if (tcgetattr(fd, &tio) < 0)
    return (-1);
  cfsetispeed(&tio, B115200);
  cfsetospeed(&tio, B115200);
  tio.c_cflag |= (CS8 | CLOCAL | CREAD);
  tio.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
  tio.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK);
  tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tio.c_oflag &= ~OPOST;
  tio.c_cc[VMIN] = 0;
  tio.c_cc[VTIME] = 1;
  if (tcsetattr(fd, TCSANOW, &tio) < 0)
    return (-1);
  tcflush(fd, TCIOFLUSH);
  return (0);
}

t_serial_ctx *serial_open(const char *device) {
  t_serial_ctx *ctx;

  ctx = (t_serial_ctx *)malloc(sizeof(t_serial_ctx));
  if (!ctx)
    return (NULL);
  memset(ctx, 0, sizeof(t_serial_ctx));
  ctx->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (ctx->fd < 0) {
    free(ctx);
    return (NULL);
  }
  if (configure_port(ctx->fd) < 0) {
    close(ctx->fd);
    free(ctx);
    return (NULL);
  }
  return (ctx);
}

void serial_close(t_serial_ctx *ctx) {
  if (!ctx)
    return;
  if (ctx->running)
    serial_stop_recv(ctx);
  if (ctx->fd >= 0)
    close(ctx->fd);
  free(ctx);
}


int serial_start_recv(t_serial_ctx *ctx) {
  (void)ctx;
  return (0);
}

void serial_stop_recv(t_serial_ctx *ctx) { (void)ctx; }

void serial_get_state(t_serial_ctx *ctx, t_car_state *out) {
  (void)ctx;
  (void)out;
}
