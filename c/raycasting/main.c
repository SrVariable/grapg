// Testing raycasting
//
// - While creating this, I developed a simple MLX setup, so I can do more future
// tests if I want to. https://gist.github.com/SrVariable/7444ae6961f40de6e6b46d5a5aeb2d06
// By the way, it is norminette compliant.
// - Apparently, hooks passed to mlx_key_hook overwrite the previous one
// - I applied and grasped the concept of Back Buffer (https://en.wikipedia.org/wiki/Multiple_buffering)
// I was familiar with the concept, however I didn't understand it completely until now.
// The implementation might be wrong, but this is a test, so who cares (maybe my future me)
// - I also kinda understand how mlx images work now
// - Figured out how to do the image_to_back_buffer but trial and error
// - After implementing image_to_image I think I could redo so_long way better,
// but I'll do it in the future because there's no time :(
// - While testing drawing grids, I noticed The image_to_image was bugged
// for other resolutions different than 800x600, for example 1024x576.
// Fortunately, I was able to fix it quick and easily.
//
// @TODO:
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
#include <bits/endian.h>
#include <math.h>

typedef union
{
	struct
	{
		double	x;
		double	y;
	};
	double	pos[2];
}				V2;

#if __BYTE_ORDER == __LITTLE_ENDIAN
enum
{
	A,
	B,
	G,
	R,
};

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
#else
enum
{
	R,
	G,
	B,
	A,
};

typedef union
{
	struct
	{
		uint8_t	r;
		uint8_t	g;
		uint8_t	b;
		uint8_t	a;
	};
	uint32_t	hex;
	uint8_t		data[4];
}				Color;
#endif

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
#define DARKGRAY ((Color){.hex = 0x303030FF})
#define GRAY ((Color){.hex = 0x606060FF})

#define TITLE "Raycasting"
#define PIXEL_SIZE 32
#define RESIZE_WINDOW 2
#define WINDOW_WIDTH ((PIXEL_SIZE) * 16 * (RESIZE_WINDOW))
#define WINDOW_HEIGHT ((PIXEL_SIZE) * 9 * (RESIZE_WINDOW))
#define BG_COLOR GRAY
#define PLAYER_COLOR RED
#define GRID_COLOR DARKGRAY

#define MAP_PATH "raycasting/.map"
#define SPRITE_PATH "sprites/"

#define ARR_SIZE 1024
typedef struct
{
	char		arr[ARR_SIZE][ARR_SIZE];
	int			cols;
	int			rows;
	mlx_image_t	*back_buffer;
	mlx_image_t	*screen;
}				Map;

typedef struct
{
	double		x;
	double		y;
	double		speed;
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
	mlx_image_t	*img[2];
	mlx_image_t	*render_img;
	int			render_img_index;
	int			render_img_count;
}				Info;

// Learnt (and stole) this trick from here: https://youtu.be/1PMf3FrFGD4?t=6233
int	proper_mod(int a, int b)
{
	return ((a % b + b) % b);
}

void	set_color(uint8_t *pixel, Color color)
{
	pixel[0] = color.r;
	pixel[1] = color.g;
	pixel[2] = color.b;
	pixel[3] = color.a;
}

void	put_pixel(mlx_image_t *img, int x, int y, Color color)
{
	uint8_t	*pixel;

	pixel = &img->pixels[(y * img->width + x) * sizeof(int)];
	set_color(pixel, color);
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
			continue ;
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
	char	buff[ARR_SIZE * ARR_SIZE] = {0};
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

int	setup_player(Info *info, Player *player)
{
	for (int i = 0; i < info->map.rows; ++i)
	{
		for (int j = 0; j < info->map.cols && info->map.arr[i][j]; ++j)
		{
			if (info->map.arr[i][j] == 'P')
			{
				player->x = i * PIXEL_SIZE;
				player->y = j * PIXEL_SIZE;
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

mlx_image_t	*load_image(Info *info, const char *path)
{
	mlx_texture_t	*tex;
	mlx_image_t		*img;

	tex = mlx_load_png(path);
	if (!tex)
		return (NULL);
	img = mlx_texture_to_image(info->mlx, tex);
	mlx_delete_texture(tex);
	if (!img)
		return (NULL);
	return (img);
}

int	setup_image(Info *info)
{
	mlx_texture_t	*tex;

	info->render_img_count = 0;
	info->img[0] = load_image(info, SPRITE_PATH"/Chopper.png");
	++info->render_img_count;
	info->img[1] = load_image(info, SPRITE_PATH"/cool.png");
	++info->render_img_count;
	if (!info->img[0] || !info->img[1])
		return (1);
	info->render_img = info->img[0];
	info->render_img_index = 0;
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
	if (setup_player(info, &info->player) != 0)
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
	Info	*info;
	uint8_t	*pixel;

	info = param;
	for (int i = 0; i < sizeof(int) * info->map.back_buffer->width * info->map.back_buffer->height; i += 4)
	{
		pixel = &info->map.back_buffer->pixels[i];
		set_color(pixel, BG_COLOR);
	}
}

void	draw_circle(mlx_image_t *img, V2 center, int radius, Color color)
{
	for (int i = 0; i < radius * 2; ++i)
	{
		for (int j = 0; j < radius * 2; ++j)
		{
			if (((j - radius) * (j - radius) + (i - radius) * (i - radius)) < (radius * radius))
				put_pixel(img, proper_mod(i + center.x, img->width), proper_mod(j + center.y, img->height), color);
		}
	}
}

void	draw_rectangle(mlx_image_t *img, V2 position, V2 size, Color color)
{
	for (int i = position.x; i < position.x + size.x; ++i)
	{
		for (int j = position.y; j < position.y + size.y; ++j)
		{
			put_pixel(img, proper_mod(i, img->width), proper_mod(j, img->height), color);
		}
	}
}

int	calculate_step(double dx, double dy)
{
	if (dx < 0)
		dx *= -1;
	if (dy < 0)
		dy *= -1;
	if (dx > dy)
		return (dx);
	return (dy);
}

void	draw_line(mlx_image_t *img, V2 start, V2 end, Color color)
{
	int	steps;
	V2	increment;
	V2	p;

	steps = calculate_step(end.x - start.x, end.y - start.y);
	increment.x = (end.x - start.x) / steps;
	increment.y = (end.y - start.y) / steps;
	p = start;
	for (int i = 0; i <= steps; ++i)
	{
		put_pixel(img, proper_mod(p.x, img->width), proper_mod(p.y, img->height), color);
		p.x += increment.x;
		p.y += increment.y;
	}
}

void	draw_grid(void *param)
{
	Info		*info;
	mlx_image_t	*back_buffer;

	info = param;
	back_buffer = info->map.back_buffer;
	for (int i = PIXEL_SIZE; i < back_buffer->height; i += PIXEL_SIZE)
		draw_line(back_buffer, (V2){0, i}, (V2){back_buffer->width, i}, GRID_COLOR);
	for (int i = PIXEL_SIZE; i < back_buffer->width; i += PIXEL_SIZE)
		draw_line(back_buffer, (V2){i, 0}, (V2){i, back_buffer->height}, GRID_COLOR);
}

// Same as mlx_get_mouse_pos, but inside the screen
void	get_mouse_pos(mlx_t *mlx, int *x, int *y)
{
	mlx_get_mouse_pos(mlx, x, y);
	if (*y > WINDOW_HEIGHT)
		*y = WINDOW_HEIGHT;
	else if (*y < 0)
		*y = 0;
	if (*x > WINDOW_WIDTH)
		*x = WINDOW_WIDTH;
	else if (*x < 0)
		*x = 0;
}

void	draw_player(void *param)
{
	Info	*info;
	Player	*player;
	int		x;
	int		y;

	info = param;
	player = &info->player;
	get_mouse_pos(info->mlx, &x, &y);
	draw_rectangle(info->map.back_buffer, (V2){player->y, player->x}, (V2){PIXEL_SIZE, PIXEL_SIZE}, PLAYER_COLOR);
	draw_line(info->map.back_buffer, (V2){player->y + PIXEL_SIZE * 0.5, player->x + PIXEL_SIZE * 0.5}, (V2){x, y}, YELLOW);
//	draw_line(info->map.back_buffer, (V2){player->y, player->x}, (V2){x, y}, YELLOW);
}

void	move_player_left(Player *player, Map *map)
{
	int	x;
	int	y;

	x = player->x / PIXEL_SIZE;
	y = (player->y - player->speed) / PIXEL_SIZE;
	if (player->x / PIXEL_SIZE - x > 0 && map->arr[x + 1][y] == '1')
		++x;
	if (player->y >= 0 && map->arr[x][y] != '1')
	{
		player->y -= player->speed;
	}
}

void	move_player_right(Player *player, Map *map)
{
	int	x;
	int	y;

	x = player->x / PIXEL_SIZE;
	y = (player->y + player->speed + PIXEL_SIZE - 1) / PIXEL_SIZE;
	if (player->x / PIXEL_SIZE - x > 0 && map->arr[x + 1][y] == '1')
		++x;
	if (player->y < (map->cols - 1) * PIXEL_SIZE && map->arr[x][y] != '1')
	{
		player->y += player->speed;
	}
}

void	move_player_down(Player *player, Map *map)
{
	int	x;
	int	y;

	x = (player->x + player->speed + PIXEL_SIZE - 1) / PIXEL_SIZE;
	y = player->y / PIXEL_SIZE;
	if (player->y / PIXEL_SIZE - y > 0 && map->arr[x][y + 1] == '1')
		++y;
	if (x < (map->rows - 1) * PIXEL_SIZE && map->arr[x][y] != '1')
	{
		player->x += player->speed;
	}
}

void	move_player_up(Player *player, Map *map)
{
	int	x;
	int	y;

	x = (player->x - player->speed) / PIXEL_SIZE;
	y = player->y / PIXEL_SIZE;
	if (player->y / PIXEL_SIZE - y > 0 && map->arr[x][y + 1] == '1')
		++y;
	if (x >= 0 && x < (map->rows - 1) * PIXEL_SIZE
		&& map->arr[x][y] != '1')
	{
		player->x -= player->speed;
	}
}

void	handle_player(void *param)
{
	Info	*info;
	Player	*player;

	info = param;
	player = &info->player;
	player->speed = 1;
	if (mlx_is_key_down(info->mlx, MLX_KEY_LEFT_SHIFT))
		player->speed *= 2;
	if (mlx_is_key_down(info->mlx, MLX_KEY_W))
	{
		move_player_up(player, &info->map);
	}
	if (mlx_is_key_down(info->mlx, MLX_KEY_S))
	{
		move_player_down(player, &info->map);
	}
	if (mlx_is_key_down(info->mlx, MLX_KEY_A))
	{
		move_player_left(player, &info->map);
	}
	if (mlx_is_key_down(info->mlx, MLX_KEY_D))
	{
		move_player_right(player, &info->map);
	}
	player->x = proper_mod(player->x, info->map.back_buffer->height);
	player->y = proper_mod(player->y, info->map.back_buffer->width);
}

void	test(mlx_key_data_t keydata, void *param)
{
	Info		*info;
	static bool	hidden = true;
	static bool	fullscreen = false;

	info = param;
	if (keydata.action == MLX_PRESS && keydata.key == MLX_KEY_C)
	{
		hidden = !hidden;
	}
	if (keydata.action == MLX_PRESS && keydata.key == MLX_KEY_PERIOD)
	{
		info->render_img_index = (info->render_img_index + 1) % info->render_img_count;
		info->render_img = info->img[info->render_img_index];
	}
	if (hidden)
	{
		mlx_set_cursor_mode(info->mlx, MLX_MOUSE_HIDDEN);
	}
	else
		mlx_set_cursor_mode(info->mlx, MLX_MOUSE_NORMAL);
}

void	image_to_image(mlx_image_t *dst, mlx_image_t *src, int x, int y)
{
	uint8_t	*src_pixel;
	uint8_t	*dst_pixel;
	V2		offset;
	int		start;

	x = proper_mod(x, dst->width);
	y = proper_mod(y, dst->height);
	src_pixel = &src->pixels[0];
	dst_pixel = &dst->pixels[0];
	offset.x = src->width * 4;
	if (src->width > dst->width)
		offset.y = dst->width / src->width;
	else
		offset.y = 1 / ((double)src->width / dst->width);
	for (int i = 0; i < src->height; ++i)
	{
		for (int j = i * offset.x; j < i * offset.x + offset.x; j += 4)
		{
			if (src_pixel[j + 3] == 0)
				continue ;
			start = offset.y * (i + y) * offset.x - i * offset.x + j + (x * 4);
			start = proper_mod(start, dst->width * dst->height * sizeof(int));
			memcpy(dst_pixel + start, src_pixel + j, 4);
		}
	}
}

void	draw_image(void *param)
{
	Info			*info;
	static double	x = 0;
	static double	y = WINDOW_HEIGHT * 0.5 - PIXEL_SIZE;

	info = param;
	image_to_image(info->map.back_buffer, info->render_img, x, y);
	draw_line(info->map.back_buffer, (V2){x + info->render_img->width * 0.5, y + info->render_img->height * 0.5}, (V2){WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.5}, LIGHTRED);
	x = proper_mod(x + 2, info->map.back_buffer->width);
}

void	draw_map(void *param)
{
	Info	*info;
	Map		*map;

	info = param;
	for (int i = 0; i < info->map.rows; ++i)
	{
		for (int j = 0; j < info->map.cols; ++j)
		{
			if (info->map.arr[i][j] == '1')
			{
				draw_rectangle(info->map.back_buffer, (V2){j * PIXEL_SIZE, i * PIXEL_SIZE}, (V2){PIXEL_SIZE, PIXEL_SIZE}, BLACK);
			}
			else if (info->map.arr[i][j] == '0' || info->map.arr[i][j] == 'P')
			{
				draw_rectangle(info->map.back_buffer, (V2){j * PIXEL_SIZE, i * PIXEL_SIZE}, (V2){PIXEL_SIZE, PIXEL_SIZE}, LIGHTBLUE);
			}
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
	mlx_loop_hook(info.mlx, draw_map, &info);
	mlx_loop_hook(info.mlx, draw_player, &info);
	mlx_loop_hook(info.mlx, draw_grid, &info);
	//mlx_loop_hook(info.mlx, draw_image, &info);
	mlx_loop_hook(info.mlx, start_drawing, &info);
	mlx_loop(info.mlx);
	if (info.is_mlx_set)
		mlx_terminate(info.mlx);
	return (0);
}
