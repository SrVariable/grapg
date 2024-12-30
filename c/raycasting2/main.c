#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#define PIXEL_SIZE 32
#define RESIZE 2
#define SCREEN_WIDTH ((PIXEL_SIZE) * 16 * (RESIZE))
#define SCREEN_HEIGHT ((PIXEL_SIZE) * 9 * (RESIZE))
#define CIRCLE_SIZE 3
#define DEG_TO_RADS(X) ((X) * ((PI) / 180))

/**
 * x: position x
 * y: position y
 * angle: angle in degrees
 */
typedef struct
{
	float	x;
	float	y;
	float	angle;
	int		size;
	int		fov;
	Vector2	ray;
} Player;

int	proper_mod(int a, int b)
{
	return ((a % b + b) % b);
}

/**
 * Fórmula: x + cos(a) * i
 *
 * Donde la posición del jugador es:
 * x + cos(a) * 0 = x
 *
 * Y lo que quiero obtener es
 *
 * x + cos(a) * i = expected;
 * i = (expected - x) / cos(a)
 *
 * El único problema está cuando cos(a) == 0
 *
 * NOTA: Esto no me da exactamente el punto de colisión, pero sí una
 * aproximación, quizá es porque no he hecho bien los cálculos, pero
 * supongo que ya me daré cuenta más adelante
 */

// Calculates the length of the next distance from X axis
int	get_x_length(Player *player, int distance)
{
	int length = 0;
	--distance;
	if (cos(DEG_TO_RADS(player->angle)) != 0)
	{
		int expected = 0;
		if (cos(DEG_TO_RADS(player->angle)) > 0)
		{
			expected = (int)(player->x / PIXEL_SIZE + 1 + distance) * PIXEL_SIZE + 1;
		}
		else
		{
			expected = (int)(player->x / PIXEL_SIZE - distance) * PIXEL_SIZE;
		}
		length = (expected - player->x) / cos(DEG_TO_RADS(player->angle));
	}
	return (length);
}

/**
 * Lo mismo que get_x_length, pero en lugar de cos, uso sin
 */

// Calculates the length of the next distance from Y axis
int	get_y_length(Player *player, int distance)
{
	int length = 0;
	--distance;
	if (sin(DEG_TO_RADS(player->angle)) != 0)
	{
		int expected = 0;
		if (sin(DEG_TO_RADS(player->angle)) > 0)
		{
			expected = (int)(player->y / PIXEL_SIZE + 1 + distance) * PIXEL_SIZE + 1;
		}
		else
		{
			expected = (int)(player->y / PIXEL_SIZE - distance) * PIXEL_SIZE;
		}
		length = (expected - player->y) / sin(DEG_TO_RADS(player->angle));
	}
	return (length);
}

/**
 * Bug #1:
 *
 * player.x = 457.4f
 * player.y = 448.4f 
 * player.angle = 276
 *
 * Fix #1:
 * Si length_y es 0, entonces asignar al siguiente punto
 *
 * Bug #2:
 *
 * player.x = 457.4f
 * player.y = 448.4f 
 * (player.angle > 336 && player.angle < 360)
 * ||
 * (player.angle > 180 && player.angle < 204)
 *
 * NOTA:
 * Fix #1 no arregla el problema, lo había malinterpretado,
 * y los cálculos del Bug #2 son incorrectos, el ángulo correcto
 * es player.angle >= 204 && player.angle <= 336
 *
 * Fix #3:
 * He eliminado Fix #1 y lo he arreglado añadiendo la condición
 * length_y != get_y_length(player, i + 1), básicamente si el siguiente
 * punto con respecto al punto y es el mismo que el actual, se sigue quedando
 * con length_x, esto parece ser que arregla el bug inicial
 */
void	draw_n_points(Player *player, int n_points)
{
	for (int i = 1; i <= n_points; ++i)
	{
		int length_x = get_x_length(player, i);
		int length_y = get_y_length(player, i);
		int length = length_x;
		if (length_y < length_x && length_y != get_y_length(player, i + 1))
		{
			length = length_y;
		}
		printf("Calculated length_x: %f %f\n", player->x + cos(DEG_TO_RADS(player->angle)) * length_x, player->y + sin(DEG_TO_RADS(player->angle)) * length_x);
		printf("Calculated length_y: %f %f\n", player->x + cos(DEG_TO_RADS(player->angle)) * length_y, player->y + sin(DEG_TO_RADS(player->angle)) * length_y);
		DrawCircle(player->x + cos(DEG_TO_RADS(player->angle)) * length, player->y + sin(DEG_TO_RADS(player->angle)) * length, 2, GREEN);
	}
}

int main(void)
{
	Player player = {
		.x = 457.4f,
		.y = 448.4f,
		.angle = 276,
		.size = 1,
		.fov = 1,
	};
	//Player player = {
	//	.x = 0.7 * PIXEL_SIZE,
	//	.y = 1.2 * PIXEL_SIZE,
	//	.angle = 50,
	//	.size = 4,
	//	.fov = 100,
	//};
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raycasting2");
	SetTargetFPS(60);
	int n_points = 1;
	float time = 0;
	bool is_paused = true;
	while (!WindowShouldClose())
	{
		time += GetFrameTime() * 60;
		if (time >= 1 && !is_paused)
		{
			player.angle = proper_mod(player.angle + 1, 360);
			time = 0;
		}
		int speed = 1;
		if (IsKeyPressed(KEY_SPACE))
			is_paused = !is_paused;
		if (IsKeyDown(KEY_LEFT_SHIFT))
			speed *= 4;
		if (IsKeyPressed(KEY_W))
			player.y -= speed;
		if (IsKeyPressed(KEY_S))
			player.y += speed;
		if (IsKeyPressed(KEY_A))
			player.x -= speed;
		if (IsKeyPressed(KEY_D))
			player.x += speed;
		if (IsKeyDown(KEY_LEFT))
			player.angle = proper_mod(player.angle - 1, 360);
		if (IsKeyDown(KEY_RIGHT))
			player.angle = proper_mod(player.angle + 1, 360);
		if (IsKeyDown(KEY_J))
			player.fov -= 1;
		if (IsKeyDown(KEY_K))
			player.fov += 1;
		int factor = 1;
		if (IsKeyDown(KEY_ONE))
			n_points = factor * 1;
		else if (IsKeyDown(KEY_TWO))
			n_points = factor * 2;
		else if (IsKeyDown(KEY_THREE))
			n_points = factor * 3;
		else if (IsKeyDown(KEY_FOUR))
			n_points = factor * 4;
		else if (IsKeyDown(KEY_FIVE))
			n_points = factor * 5;


		BeginDrawing();
		ClearBackground(GetColor(0x303030FF));

		// Draw grids
		for (int i = 0; i < SCREEN_HEIGHT; i += PIXEL_SIZE)
		{
			DrawLine(0, i, SCREEN_WIDTH, i, BLACK);
		}
		for (int i = 0; i < SCREEN_WIDTH; i += PIXEL_SIZE)
		{
			DrawLine(i, 0, i, SCREEN_HEIGHT, BLACK);
		}

		// Draw player
		DrawCircle(player.x, player.y, player.size, RED);

		// Draw player main ray fov
		player.ray = (Vector2){
			.x = player.x + cos(DEG_TO_RADS(player.angle)) * player.fov,
			.y = player.y + sin(DEG_TO_RADS(player.angle)) * player.fov,
		};
		printf("Player: %f %f | Scaled: %f %f\n", player.x, player.y, player.x / PIXEL_SIZE, player.y / PIXEL_SIZE);
		printf("Ray end: %f %f | Scaled: %f %f | Expected: %d %d\n", player.ray.x, player.ray.y, player.ray.x / PIXEL_SIZE, player.ray.y / PIXEL_SIZE, (int)(player.x / PIXEL_SIZE) + 1, (int)(player.y / PIXEL_SIZE) + 1);
		printf("Angle: %f\n", player.angle);
		DrawLine(player.x, player.y, player.ray.x, player.ray.y, GetColor(0xFFFFFFAA));

		/**
		 * Thinking:
		 * La siguiente posición siempre es
		 * (int)(player.x/PIXEL_SIZE) + 1
		 */
		//{
		//	int saved = -1;
		//	for (int i = 0; i < 1000; ++i)
		//	{
		//		int expected = (int)(player.x / PIXEL_SIZE) + 1;
		//		int scaled = (player.x + cos(DEG_TO_RADS(player.angle)) * i) / PIXEL_SIZE;
		//		if (scaled >= expected)
		//		{
		//			saved = i;
		//			break;
		//		}
		//	}
		//	printf("%f: %d -> %f\n", player.angle, saved, player.x + cos(DEG_TO_RADS(player.angle)) * saved);
		//	//DrawCircle(player.x + cos(DEG_TO_RADS(player.angle)) * saved, player.y + sin(DEG_TO_RADS(player.angle)) * saved, 3, GetColor(0xAAAAAAFF));
		//}

		/**
		 * Esto es para la Y
		 */
		//{
		//	int saved = 0;
		//	for (int i = 0; i < 1000; ++i)
		//	{
		//		int expected = (int)(player.y / PIXEL_SIZE) + 1;
		//		int scaled = (player.y + sin(DEG_TO_RADS(player.angle)) * i) / PIXEL_SIZE;
		//		if (scaled >= expected)
		//		{
		//			saved = i;
		//			break;
		//		}
		//	}
		//	printf("%f: %d -> %f\n", player.angle, saved, player.y + sin(DEG_TO_RADS(player.angle)) * saved);
		//	//DrawCircle(player.x + cos(DEG_TO_RADS(player.angle)) * saved, player.y + sin(DEG_TO_RADS(player.angle)) * saved, 3, GetColor(0xAAAAAAFF));
		//}

		draw_n_points(&player, n_points);
		EndDrawing();
	}
	CloseWindow();
	return (0);
}
