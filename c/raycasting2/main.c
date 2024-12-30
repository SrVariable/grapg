#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#define PIXEL_SIZE 128
#define RESIZE 0.5
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

int main(void)
{
	Player player = {
		.x = 0.7 * PIXEL_SIZE,
		.y = 1.2 * PIXEL_SIZE,
		.angle = 50,
		.size = 10,
		.fov = 100,
	};
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raycasting2");
	SetTargetFPS(60);
	while (!WindowShouldClose())
	{
		int speed = 1;
		if (IsKeyDown(KEY_LEFT_SHIFT))
			speed *= 4;
		if (IsKeyDown(KEY_W))
			player.y -= speed;
		if (IsKeyDown(KEY_S))
			player.y += speed;
		if (IsKeyDown(KEY_A))
			player.x -= speed;
		if (IsKeyDown(KEY_D))
			player.x += speed;
		if (IsKeyDown(KEY_LEFT))
			player.angle = proper_mod(player.angle - 1, 360);
		if (IsKeyDown(KEY_RIGHT))
			player.angle = proper_mod(player.angle + 1, 360);
		if (IsKeyDown(KEY_J))
			player.fov -= 1;
		if (IsKeyDown(KEY_K))
			player.fov += 1;


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
		printf("Player: %f %f | %f %f\n", player.x, player.y, player.x / PIXEL_SIZE, player.y / PIXEL_SIZE);
		printf("Ray end: %f %f | %f %f || %d %d\n", player.ray.x, player.ray.y, player.ray.x / PIXEL_SIZE, player.ray.y / PIXEL_SIZE, (int)(player.x / PIXEL_SIZE) + 1, (int)(player.y / PIXEL_SIZE) + 1);
		DrawLine(player.x, player.y, player.ray.x, player.ray.y, YELLOW);

		/**
		 * Thinking:
		 * La siguiente posición siempre es
		 * (int)(player.x/PIXEL_SIZE) + 1
		 */
		{
			int saved = -1;
			for (int i = 0; i < 1000; ++i)
			{
				int expected = (int)(player.x / PIXEL_SIZE) + 1;
				int scaled = (player.x + cos(DEG_TO_RADS(player.angle)) * i) / PIXEL_SIZE;
				if (scaled >= expected)
				{
					saved = i;
					break;
				}
			}
			printf("%f: %d -> %f\n", player.angle, saved, player.x + cos(DEG_TO_RADS(player.angle)) * saved);
			//DrawCircle(player.x + cos(DEG_TO_RADS(player.angle)) * saved, player.y + sin(DEG_TO_RADS(player.angle)) * saved, 3, GetColor(0xAAAAAAFF));
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
		{
			int length = -1;
			if (cos(DEG_TO_RADS(player.angle)) != 0)
			{
				int expected = 0;
				int distance = 1;
				if (cos(DEG_TO_RADS(player.angle)) > 0)
				{
					expected = (int)(player.x / PIXEL_SIZE + 1 + distance) * PIXEL_SIZE;
				}
				else
				{
					expected = (int)(player.x / PIXEL_SIZE - distance) * PIXEL_SIZE;
				}
				length = (expected - player.x) / cos(DEG_TO_RADS(player.angle));
			}
			printf("step: %d -> %f\n", length, player.x + cos(DEG_TO_RADS(player.angle)) * length);
			DrawCircle(player.x + cos(DEG_TO_RADS(player.angle)) * length, player.y + sin(DEG_TO_RADS(player.angle)) * length, 3, GetColor(0xFFBBFFFF));
		}

		// Esto sirve para la Y
		{
			int saved = -1;
			for (int i = 0; i < 1000; ++i)
			{
				int expected = (int)(player.y / PIXEL_SIZE) + 1;
				int scaled = (player.y + sin(DEG_TO_RADS(player.angle)) * i) / PIXEL_SIZE;
				if (scaled >= expected)
				{
					saved = i;
					break;
				}
			}
			printf("%f: %d -> %f\n", player.angle, saved, player.y + sin(DEG_TO_RADS(player.angle)) * saved);
			DrawCircle(player.x + cos(DEG_TO_RADS(player.angle)) * saved, player.y + sin(DEG_TO_RADS(player.angle)) * saved, 3, BLUE);
		}
		EndDrawing();
	}
	CloseWindow();
	return (0);
}
