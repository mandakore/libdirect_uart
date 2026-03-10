#include "serial_engine.h"
#include <string.h>

void rb_init(t_ring_buffer *rb)
{
  memset(rb->buf, 0, RB_SIZE);
  rb->head = 0;
  rb->tail = 0;
  pthread_mutex_init(&rb->mutex, NULL);
}


int rb_available(t_ring_buffer *rb)
{ 
  int h;
  int t;

  pthread_mutex_lock(&rb->mutex);
  h = rb->head;
  t = rb->tail;
  pthread_mutex_unlock(&rb->mutex);
  return ((h - t + RB_SIZE) % RB_SIZE);
}


int rb_write(t_ring_buffer *rb, const uint8_t *data, int len) 
{
  int i;
  int next;

  i = 0;
  pthread_mutex_lock(&rb->mutex);
  while (i < len) 
  {
    next = (rb->head + 1) % RB_SIZE;
    if (next == rb->tail)
      break;
    rb->buf[rb->head] = data[i];
    rb->head = next;
    i++;
  }
  pthread_mutex_unlock(&rb->mutex);
  return (i);
}


int rb_read(t_ring_buffer *rb, uint8_t *dest, int len) 
{
  int i;

  i = 0;
  pthread_mutex_lock(&rb->mutex);
  while (i < len && rb->tail != rb->head)
  {
    dest[i] = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % RB_SIZE;
    i++;
  }
  pthread_mutex_unlock(&rb->mutex);
  return (i);
}



int rb_peek(t_ring_buffer *rb, uint8_t *dest, int len) 
{
  int i;
  int pos;

  i = 0;
  pthread_mutex_lock(&rb->mutex);
  pos = rb->tail;
  while (i < len && pos != rb->head) 
  {
    dest[i] = rb->buf[pos];
    pos = (pos + 1) % RB_SIZE;
    i++;
  }
  pthread_mutex_unlock(&rb->mutex);
  return (i);
}



void rb_discard(t_ring_buffer *rb, int len) 
{
  int i;

  i = 0;
  pthread_mutex_lock(&rb->mutex);
  while (i < len && rb->tail != rb->head) 
  {
    rb->tail = (rb->tail + 1) % RB_SIZE;
    i++;
  }
  pthread_mutex_unlock(&rb->mutex);
}
