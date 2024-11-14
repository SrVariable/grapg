// Testing raycasting
//
// - While creating this, I developed a simple MLX setup, so I can do more future
// tests if I want to. https://gist.github.com/SrVariable/7444ae6961f40de6e6b46d5a5aeb2d06
// By the way, it is norminette compliant.
//
// - Apparently, hooks passed to mlx_key_hook overwrite the previous one
//
// - I applied and grasped the concept of Back Buffer (https://en.wikipedia.org/wiki/Multiple_buffering)
// I was familiar with the concept, however I didn't understand it completely until now.
// The implementation might be wrong, but this is a test, so who cares (maybe my future me)
//
// - I also kinda understand how mlx images work now
//
// - Figured out how to do the image_to_back_buffer but trial and error
//
// @TODO:
// - Create image_to_image based on image_to_back_buffer that accepts a position
// so the image will be placed in that position
// - Implement raycasting???
// - Create a custom mlx_resize_image

#include "MLX42.h"
#include "stdbool.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef union
{
	struct
	{
		uint8_t	a;
		uint8_t	b;
		uint8_t	g;
		uint8_t	r;
	};
	uint32_t	hex;
	uint8_t		data[4];
}				Color;

#define WHITE ((Color){.r = 255, .g = 255, .b = 255, .a = 255})
#define BLACK ((Color){.r = 0, .g = 0, .b = 0, .a = 255})
#define RED ((Color){.r = 255, .g = 0, .b = 0, .a = 255})
#define GREEN ((Color){.r = 0, .g = 255, .b = 0, .a = 255})
#define BLUE ((Color){.r = 0, .g = 0, .b = 255, .a = 255})
#define YELLOW ((Color){.r = 255, .g = 255, .b = 0, .a = 255})
#define LIGHTRED ((Color){.r = 255, .g = 127, .b = 127, .a = 255})
#define LIGHTGREEN ((Color){.r = 127, .g = 255, .b = 127, .a = 255})
#define LIGHTBLUE ((Color){.r = 127, .g = 127, .b = 255, .a = 255})
#define LIGHTYELLOW ((Color){.r = 255, .g = 255, .b = 127, .a = 255})

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TITLE "Raycasting"
#define PIXEL_SIZE 64
#define PLAYER_COLOR LIGHTRED

#define MAP_PATH "raycasting/.map"
#define PNG_PATH "sprites/cool.png"

typedef struct
{
	char		arr[1024][1024];
	int			cols;
	int			rows;
	mlx_image_t	*back_buffer;
	mlx_image_t	*screen;
}				Map;

typedef struct
{
	double	x;
	double	y;
}				Player;

typedef struct
{
	int	x;
	int	y;
	int	old_x;
	int	old_y;
}				Mouse;

typedef struct
{
	mlx_t		*mlx;
	bool		is_mlx_set;
	Player		player;
	Map			map;
	Mouse		mouse;
	mlx_image_t	*img;
}				Info;

void	custom_put_pixel(mlx_image_t *img, int x, int y, Color color)
{
	uint8_t	*pixel;

	pixel = &img->pixels[(y * img->width + x) * sizeof(int)];
	pixel[0] = color.r;
	pixel[1] = color.g;
	pixel[2] = color.b;
	pixel[3] = color.a;
}

void	hook_close_window(void *param)
{
	mlx_t	*mlx;

	mlx = param;
	if (mlx_is_key_down(mlx, MLX_KEY_Q)
		|| mlx_is_key_down(mlx, MLX_KEY_ESCAPE))
		mlx_close_window(mlx);
}

mlx_t	*setup_mlx(void)
{
	mlx_t	*mlx;

	mlx = mlx_init(WINDOW_WIDTH, WINDOW_HEIGHT, TITLE, false);
	if (!mlx)
		return (NULL);
	mlx_loop_hook(mlx, hook_close_window, mlx);
	return (mlx);
}

void	fill_map(Map *map, ssize_t bytes_read, char *buff)
{
	int		r;
	int		c;

	r = 0;
	c = 0;
	for (ssize_t i = 0; i < bytes_read; ++i)
	{
		if (buff[i] == '\n')
		{
			map->arr[r][c] = '\0';
			r += 1;
			if (c > map->cols)
				map->cols = c;
			c = 0;
			continue;
		}
		map->arr[r][c] = buff[i];
		++c;
	}
	map->rows = r;
}

int	create_screen(Info *info, Map *map)
{
	map->screen = mlx_new_image(info->mlx, WINDOW_WIDTH, WINDOW_HEIGHT);
	if (!map->screen)
		return (1);
	map->back_buffer = mlx_new_image(info->mlx, WINDOW_WIDTH, WINDOW_HEIGHT);
	if (!map->back_buffer)
		return (2);
	return (0);
}

int	setup_map(Info *info, Map *map)
{
	char	buff[1024] = {0};
	int		fd;
	ssize_t	bytes_read;

	if (create_screen(info, map) != 0)
		return (1);
	fd = open(MAP_PATH, O_RDONLY);
	if (fd < 0)
		return (2);
	bytes_read = read(fd, buff, sizeof(buff));
	close(fd);
	if (bytes_read < 0)
		return (3);
	map->cols = 0;
	fill_map(map, bytes_read, buff);
	return (0);
}

int	setup_player(Map *map, Player *player)
{
	for (int i = 0; i < map->rows; ++i)
	{
		for (int j = 0; j < map->cols && map->arr[i][j]; ++j)
		{
			if (map->arr[i][j] == 'P')
			{
				player->x = i;
				player->y = j;
				return (0);
			}
		}
	}
	return (1);
}

void	setup_mouse(mlx_t *mlx, Mouse *mouse)
{
	mlx_set_cursor_mode(mlx, MLX_MOUSE_HIDDEN);
	mlx_set_mouse_pos(mlx, WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.5);
	mouse->old_x = WINDOW_WIDTH * 0.5;
	mouse->old_y = WINDOW_HEIGHT * 0.5;
}

int	setup_image(Info *info)
{
	mlx_texture_t	*tex;
	mlx_image_t		*img;

	tex = mlx_load_png(PNG_PATH);
	if (!tex)
		return (1);
	info->img = mlx_texture_to_image(info->mlx, tex);
	mlx_delete_texture(tex);
	if (!info->img)
		return (2);
	return (0);
}

int	setup_info(Info *info)
{
	info->is_mlx_set = false;
	info->mlx = setup_mlx();
	if (!info->mlx)
		return (1);
	info->is_mlx_set = true;
	if (setup_map(info, &info->map) != 0)
		return (2);
	if (setup_player(&info->map, &info->player) != 0)
		return (3);
	if (setup_image(info) != 0)
		return (4);
	setup_mouse(info->mlx, &info->mouse);
	return (0);
}

void	start_drawing(void *param)
{
	Info		*info;
	mlx_image_t	*back_buffer;
	mlx_image_t	*screen;

	info = param;
	screen = info->map.screen;
	back_buffer = info->map.back_buffer;
	memcpy(screen->pixels, back_buffer->pixels, sizeof(int) * screen->width * screen->height);
}

void	clear_background(void *param)
{
	Info *info;

	info = param;
	for (int i = 0; i < sizeof(int) * info->map.back_buffer->width * info->map.back_buffer->height; i += 4)
	{
		info->map.back_buffer->pixels[i + 0] = 0x30;
		info->map.back_buffer->pixels[i + 1] = 0x30;
		info->map.back_buffer->pixels[i + 2] = 0x30;
		info->map.back_buffer->pixels[i + 3] = 0xFF;
	}
}

void	draw_player(void *param)
{
	Info	*info;
	Player	*player;
	int		x;
	int		y;

	info = param;
	player = &info->player;
	x = player->x * PIXEL_SIZE;
	y = player->y * PIXEL_SIZE;
	if (x < 0 || x + PIXEL_SIZE > WINDOW_HEIGHT
		|| y < 0 || y + PIXEL_SIZE > WINDOW_WIDTH)
		return ;
	for (int i = x; i < x + PIXEL_SIZE; ++i)
	{
		for (int j = y; j < y + PIXEL_SIZE; ++j)
		{
			custom_put_pixel(info->map.back_buffer, j, i, PLAYER_COLOR);
		}
	}
}

void	handle_player(void *param)
{
	Info	*info;
	Player	*player;
	double	speed;

	info = param;
	player = &info->player;
	speed = 6 * info->mlx->delta_time;
	if (mlx_is_key_down(info->mlx, MLX_KEY_LEFT_SHIFT))
		speed *= 2;
	if (mlx_is_key_down(info->mlx, MLX_KEY_W))
		player->x -= speed;
	else if (mlx_is_key_down(info->mlx, MLX_KEY_S))
		player->x += speed;
	if (mlx_is_key_down(info->mlx, MLX_KEY_A))
		player->y -= speed;
	else if (mlx_is_key_down(info->mlx, MLX_KEY_D))
		player->y += speed;
}

void	test(mlx_key_data_t keydata, void *param)
{
	Info	*info;

	info = param;
	static bool	hidden = true;
	static bool	fullscreen = false;
	if (keydata.action == MLX_PRESS && keydata.key == MLX_KEY_C)
	{
		hidden = !hidden;
	}
	if (keydata.action == MLX_PRESS && keydata.key == MLX_KEY_PERIOD)
	{
		info->player.x = 0;
		info->player.y = 0;
	}
	if (hidden)
	{
		mlx_set_cursor_mode(info->mlx, MLX_MOUSE_HIDDEN);
		mlx_set_mouse_pos(info->mlx, WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.5);
	}
	else
		mlx_set_cursor_mode(info->mlx, MLX_MOUSE_NORMAL);
}

void	image_to_back_buffer(void *param)
{
	Info	*info;
	uint8_t	*src_pixel;
	uint8_t	*dst_pixel;

	info = param;
	src_pixel = &info->img->pixels[0];
	dst_pixel = &info->map.back_buffer->pixels[0];

	// These variables might need a rename
	int	offset = info->img->width * 4;
	double	width = 8.0 / info->img->width * 100;

	for (int i = 0; i < info->img->height; ++i)
	{
		for (int j = i * offset; j < i * offset + offset; j += 4)
		{
			if (src_pixel[j + 3] == 0)
				continue;
			int	start = width * i * offset - i * offset + j;
			dst_pixel[start + 0] = src_pixel[j + 0];
			dst_pixel[start + 1] = src_pixel[j + 1];
			dst_pixel[start + 2] = src_pixel[j + 2];
			dst_pixel[start + 3] = src_pixel[j + 3];
		}
	}
}

int	main(void)
{
	Info	info;
	int		error;

	if (setup_info(&info) != 0)
	{
		if (info.is_mlx_set)
			mlx_terminate(info.mlx);
		return (1);
	}
	mlx_image_to_window(info.mlx, info.map.screen, 0, 0);
	mlx_loop_hook(info.mlx, handle_player, &info);
	mlx_key_hook(info.mlx, test, &info);
	mlx_loop_hook(info.mlx, clear_background, &info);
	mlx_loop_hook(info.mlx, draw_player, &info);
	mlx_loop_hook(info.mlx, image_to_back_buffer, &info);
	mlx_loop_hook(info.mlx, start_drawing, &info);
	mlx_loop(info.mlx);
	if (info.is_mlx_set)
		mlx_terminate(info.mlx);
	return (0);
}
