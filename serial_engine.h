/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serial_engine.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: atashiro                                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/10 12:50:00 by atashiro          #+#    #+#             */
/*   Updated: 2026/03/10 12:50:00 by atashiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERIAL_ENGINE_H
# define SERIAL_ENGINE_H

# include <pthread.h>
# include <stdint.h>

# define PACKET_HEADER		0x42
# define PACKET_SIZE		29
# define SENSOR_COUNT		3

typedef struct s_car_state
{
	float	x;
	float	y;
	float	speed;
	float	angle;
	float	sensor_dist[SENSOR_COUNT];
}	t_car_state;

typedef struct s_serial_ctx
{
	int				fd;
	int				running;
	pthread_t		thread;
	pthread_mutex_t	mutex;
	t_car_state		state;
}	t_serial_ctx;

t_serial_ctx	*serial_open(const char *device);
void			serial_close(t_serial_ctx *ctx);
int				serial_start_recv(t_serial_ctx *ctx);
void			serial_stop_recv(t_serial_ctx *ctx);
void			serial_get_state(t_serial_ctx *ctx, t_car_state *out);

#endif
