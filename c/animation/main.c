// To understand better how the image and pixel works on MLX
// I might refactor this later
#include "MLX42.h"
#include "BFL.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define SOFT

#ifndef SOFT
# define RED (t_color){.r = 255, .g = 0, .b = 0, .a = 255}
# define GREEN (t_color){.r = 0, .g = 255, .b = 0, .a = 255}
# define BLUE (t_color){.r = 0, .g = 0, .b = 255, .a = 255}
#else
# define RED (t_color){.r = 255, .g = 63, .b = 63, .a = 255}
# define GREEN (t_color){.r = 63, .g = 255, .b = 63, .a = 255}
# define BLUE (t_color){.r = 63, .g = 63, .b = 255, .a = 255}
#endif
#define WHITE (t_color){.r = 255, .g = 255, .b = 255, .a = 255}
#define BLACK (t_color){.r = 0, .g = 0, .b = 0, .a = 255}

typedef union
{
	struct
	{
		float x;
		float y;
	};
	struct
	{
		uint32_t ux;
		uint32_t uy;
	};
	float data[2];
} t_v2;

typedef union
{
	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};
	uint8_t data[4];
	unsigned long hex;
} t_color;

int	proper_mod(int a, int b)
{
	return ((a % b + b) % b);
}

void	hook_control_key(mlx_key_data_t keydata, void *param)
{
	mlx_t *mlx;

	mlx = param;
	if (keydata.key == MLX_KEY_Q || keydata.key == MLX_KEY_ESCAPE)
	{
		bfl_printf("Closing the window\n");
		mlx_close_window(mlx);
	}
}

void	pixel_to_image(mlx_image_t *image, t_v2 v2, t_color color)
{
	uint8_t *pixel_start;

	pixel_start = &image->pixels[(v2.ux * image->width + v2.uy) * sizeof(int32_t)];
	pixel_start[0] = color.data[0];
	pixel_start[1] = color.data[1];
	pixel_start[2] = color.data[2];
	pixel_start[3] = color.data[3];
}

mlx_image_t	*rect_image(mlx_t *mlx, int32_t width, int32_t height, t_color color)
{
	mlx_image_t *image;

	image = mlx_new_image(mlx, width, height);
	if (!image)
		return (NULL);
	for (int32_t i = 0; i < width; ++i)
	{
		for (int32_t j = 0; j < height; ++j)
		{
			pixel_to_image(image, (t_v2){.ux = j, .uy = i}, color);
		}
	}
	return (image);
}

mlx_image_t	*circle_image(mlx_t *mlx, t_v2 center, int32_t radius, t_color color)
{
	mlx_image_t *image;

	image = mlx_new_image(mlx, 2 * radius, 2 * radius);
	if (!image)
		return (NULL);
	for (int32_t i = 0; i < radius * 2; ++i)
	{
		for (int32_t j = 0; j < radius * 2; ++j)
		{
			if (((j - radius) * (j - radius) + (i - radius) * (i - radius)) < (radius * radius))
				pixel_to_image(image, (t_v2){.ux = j + center.uy, .uy = i + center.ux}, color);
		}
	}
	return (image);
}

void	change_color(void *param)
{
	mlx_image_t *img;
	static int c = 0;
	static int frame = 0;
	t_color color[2];

	img = param;
	color[0] = RED;
	color[1] = BLUE;
	if (frame++ % 30 == 0)
	{
		for (uint32_t i = 0; i < img->width; ++i)
		{
			for (uint32_t j = 0; j < img->height; ++j)
			{
				pixel_to_image(img, (t_v2){.ux = j, .uy = i}, color[c % 2]);
			}
		}
		++c;
		img->instances[0].x = proper_mod((img->instances[0].x - 100), SCREEN_WIDTH);
	}
	if (frame == 60)
		frame = 0;
}

void	change_circle_color(void *param)
{
	mlx_image_t *img;
	static int c = 0;
	t_color color[3];

	img = param;
	color[0] = RED;
	color[1] = BLUE;
	color[2] = GREEN;
	uint32_t radius = img->height * 0.5;
	for (uint32_t i = 0; i < img->width; ++i)
	{
		for (uint32_t j = 0; j < img->height; ++j)
		{
			if (((j - radius) * (j - radius) + (i - radius) * (i - radius)) < (radius * radius))
				pixel_to_image(img, (t_v2){.ux = j, .uy = i}, color[c % 3]);
		}
	}
	++c;
}

void	change_circle_position(void *param)
{
	mlx_image_t *img = param;
	static int i = 0;
	static int j = 0;
	static bool is_reverse = false;

	if (++i == 60)
	{
		i = 0;
		++j;
	}
	if (j == 10)
	{
		j = 0;
		is_reverse = !is_reverse;
	}
	int	increase_position = 10;
	if (is_reverse)
		increase_position *= -1;
	img->instances[0].y = proper_mod((img->instances[0].y - increase_position), SCREEN_HEIGHT);
	img->instances[0].x = proper_mod((img->instances[0].x + increase_position * 0.5), SCREEN_HEIGHT + img->width);
}

void	circle_animation(void *param)
{
	mlx_image_t *img = param;
	change_circle_color(img);
	change_circle_position(img);
}

void	change_rect_color(void *param)
{
	mlx_image_t *img;
	static int c = 0;
	static int frame = 0;
	t_color color[2];

	img = param;
	color[0] = WHITE;
	color[1] = BLACK;
	if (frame++ % 60 == 0)
	{
		for (uint32_t i = 0; i < img->width; ++i)
		{
			for (uint32_t j = 0; j < img->height; ++j)
			{
				pixel_to_image(img, (t_v2){.ux = j, .uy = i}, color[c % 2]);
			}
		}
		++c;
	}
}

void	change_rect_position(void *param)
{
	mlx_image_t *img = param;
	static int i = 0;
	static bool is_reverse = false;

	if (++i == 30)
	{
		i = 0;
		is_reverse = !is_reverse;
	}
	int	increase_position = 5;
	if (is_reverse)
		increase_position *= -2;
	img->instances[0].x = proper_mod((img->instances[0].x + increase_position), SCREEN_WIDTH);
}

void	rect_animation(void *param)
{
	mlx_image_t *img = param;
	change_rect_color(img);
	change_rect_position(img);
}

int32_t	main(void)
{
	mlx_t *mlx;

	mlx = mlx_init(SCREEN_WIDTH, SCREEN_HEIGHT, "Animation", false);
	if (!mlx)
	{
		bfl_fprintf(2, "Couldn't init mlx\n");
		return (1);
	}
	const int32_t	width = 64;
	const int32_t	height = 128;
	mlx_image_t	*rect   = rect_image(mlx, width * 2, height * 2, BLUE);
	mlx_image_t	*circle = circle_image(mlx, (t_v2){.ux = 0, .uy = 0}, width, GREEN);
	mlx_image_t	*rect2  = rect_image(mlx, width, height, RED);
	mlx_key_hook(mlx, hook_control_key, mlx);
	mlx_image_to_window(mlx, rect, SCREEN_WIDTH * 0.5 - width, SCREEN_HEIGHT * 0.5 - height);
	mlx_image_to_window(mlx, circle, SCREEN_WIDTH * 0.5 - width, SCREEN_HEIGHT * 0.5 - height);
	mlx_image_to_window(mlx, rect2, SCREEN_WIDTH * 0.5 - width, SCREEN_HEIGHT * 0.5 - height);
	mlx_loop_hook(mlx, change_color, rect);
	mlx_loop_hook(mlx, rect_animation, rect2);
	mlx_loop_hook(mlx, circle_animation, circle);
	mlx_loop(mlx);
	mlx_terminate(mlx);
	return (0);
}
