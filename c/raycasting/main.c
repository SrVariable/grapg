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
// @TODO:
// - Think a way to have an image and put it into another image.
// For example: I load a Chopper.png with mlx_load_png, then convert the
// texture as an image, then put the pixels of Chopper wherever I want to in
// back_buffer, which will be later copied into screen
// - Implement raycasting???

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
#define LIGHT_RED ((Color){.r = 255, .g = 127, .b = 127, .a = 255})
#define LIGHT_GREEN ((Color){.r = 127, .g = 255, .b = 127, .a = 255})
#define LIGHT_BLUE ((Color){.r = 127, .g = 127, .b = 255, .a = 255})

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TITLE "Raycasting"
#define PIXEL_SIZE 64
#define PLAYER_COLOR LIGHT_BLUE

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
	mlx_t	*mlx;
	bool	is_mlx_set;
	Player	player;
	Map		map;
	Mouse	mouse;
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
	fd = open("raycasting/.map", O_RDONLY);
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
	setup_mouse(info->mlx, &info->mouse);
	return (0);
}

#define ENABLE 0
#if ENABLE

void	raycasting(void *param)
{
	Info		*info;
	mlx_image_t	*back_buffer;
	mlx_image_t	*screen;

	info = param;
	screen = info->map.screen;
	back_buffer = info->map.back_buffer;
	memcpy(screen->pixels, back_buffer->pixels, sizeof(int) * screen->width * screen->height);
}

void	change_buff(void *param)
{
	Info	*info = param;
	mlx_image_t	*back_buffer = info->map.back_buffer;
	static int j = 0;

	for (int i = 0; i < back_buffer->height * 0.3; ++i)
	{
		mlx_put_pixel(back_buffer, j, i, 0x0000FFFF);
	}
	if (++j == back_buffer->width)
	{
		j = 0;
		memset(back_buffer->pixels, 255, sizeof(int) * back_buffer->width * back_buffer->height);
	}
}

void	change_buff2(void *param)
{
	Info	*info = param;
	mlx_image_t	*back_buffer = info->map.back_buffer;
	static int j = 0;

	if (j < back_buffer->width * 0.6)
	{
		for (int i = back_buffer->height * 0.3; i < back_buffer->height * 0.6; ++i)
		{
			mlx_put_pixel(back_buffer, j, i, 0x00FF00FF);
		}
	}
	if (++j == back_buffer->width)
	{
		j = 0;
		memset(back_buffer->pixels, 255, sizeof(int) * back_buffer->width * back_buffer->height);
	}
}

void	change_buff3(void *param)
{
	Info	*info = param;
	mlx_image_t	*back_buffer = info->map.back_buffer;
	static int j = 0;

	if (j < back_buffer->width * 0.3)
	{
		for (int i = back_buffer->height * 0.6; i < back_buffer->height; ++i)
		{
			mlx_put_pixel(back_buffer, j, i, 0xFF0000FF);
		}
	}
	if (++j == back_buffer->width)
	{
		j = 0;
		memset(back_buffer->pixels, 255, sizeof(int) * back_buffer->width * back_buffer->height);
	}
}

#else

void	raycasting(void *param)
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
			// @TODO:
			// Now that I kinda understand how the image works,
			// I might need to replace this
			//mlx_put_pixel(info->map.back_buffer, j, i, PLAYER_COLOR);
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
	if (hidden)
	{
		mlx_set_cursor_mode(info->mlx, MLX_MOUSE_HIDDEN);
		mlx_set_mouse_pos(info->mlx, WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.5);
	}
	else
		mlx_set_cursor_mode(info->mlx, MLX_MOUSE_NORMAL);
	if (keydata.key == MLX_KEY_P)
	{
		int count = 0;
		for (int i = 0; i < (sizeof(int) * info->map.screen->width * info->map.screen->height); ++i)
		{
			// Filtering the indexes where the player is
			// If the player has the same color as the background, which
			// white, this won't do anything
			if (info->map.back_buffer->pixels[i] != 255 && i % 4 == 0
				|| info->map.back_buffer->pixels[i] != 255 && i % 4 == 1
				|| info->map.back_buffer->pixels[i] != 255 && i % 4 == 2
				|| info->map.back_buffer->pixels[i] != 255 && i % 4 == 3)
			{
				printf("R component at %d: %d\n", i, info->map.back_buffer->pixels[i]); // r
				printf("G component at %d: %d\n", i + 1, info->map.back_buffer->pixels[i + 1]); // g
				printf("B component at %d: %d\n", i + 2, info->map.back_buffer->pixels[i + 2]); // b
				printf("A component at %d: %d\n", i + 3, info->map.back_buffer->pixels[i + 3]); // a
				++count;
			}
		}
		printf("%d\n", count); // The count is PIXEL_SIZE * PIXEL_SIZE
	}
}

void	handle_mouse(void *param)
{
	Info	*info;
	Mouse	*mouse;

	info = param;
	mouse = &info->mouse;
	mlx_get_mouse_pos(info->mlx, &mouse->x, &mouse->y);
	//if (mouse->y > mouse->old_y)
	//	printf("Mouse moved to the right\n");
	//else if (mouse->y < mouse->old_y)
	//	printf("Mouse moved to the left\n");
	//mlx_set_mouse_pos(info->mlx, mouse->old_x, mouse->old_y);
}

#endif

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
#if ENABLE
	mlx_loop_hook(info.mlx, change_buff, &info);
	mlx_loop_hook(info.mlx, change_buff2, &info);
	mlx_loop_hook(info.mlx, change_buff3, &info);
#else
	mlx_loop_hook(info.mlx, handle_player, &info);
	mlx_loop_hook(info.mlx, handle_mouse, &info);
	mlx_key_hook(info.mlx, test, &info);
	mlx_loop_hook(info.mlx, clear_background, &info);
	mlx_loop_hook(info.mlx, draw_player, &info);
#endif
	mlx_loop_hook(info.mlx, raycasting, &info);
	mlx_loop(info.mlx);
	if (info.is_mlx_set)
		mlx_terminate(info.mlx);
	return (0);
}
