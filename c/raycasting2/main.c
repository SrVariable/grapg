#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define PIXEL_SIZE 32
#define RESIZE 2
#define SCREEN_WIDTH ((PIXEL_SIZE) * 16 * (RESIZE))
#define SCREEN_HEIGHT ((PIXEL_SIZE) * 9 * (RESIZE))
#define COLS (16 * (RESIZE))
#define ROWS (9 * (RESIZE))
#define DEG_TO_RADS(X) ((X) * ((PI) / 180))
#define RADS_TO_DEG(X) ((X) * (180 / (PI)))

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
 * Fórmula:
 * x + cos(a) * i
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
 * NOTA:
 * Esto no me da exactamente el punto de colisión, pero sí una
 * aproximación, quizá es porque no he hecho bien los cálculos, pero
 * supongo que ya me daré cuenta más adelante
 *
 * NOTA:
 * He modificado la comprobación de la división entre 0, porque lo estaba
 * haciendo igualmente, ahora compruebo si el ángulo es 90 o 270, ya que
 * son los que dan 0 al calcular su coseno
 *
 * NOTA:
 * He redondeado el cálculo de expected, para que al detectar las colisiones
 * pille el valor que quiero
 */

// Calculates the length of the next distance from X axis
int	get_x_length(Player *player, int distance, float angle)
{
	if (((int)(angle * 180 / PI)) == 90
		|| ((int)(angle * 180 / PI)) == 270)
	{
		return (0);
	}
	int length = 0;
	int expected = 0;
	if (cos(angle) > 0)
	{
		expected = (int)(roundf(player->y / PIXEL_SIZE + distance)) * PIXEL_SIZE + 1;
	}
	else
	{
		expected = (int)(roundf(player->y / PIXEL_SIZE - distance + 1)) * PIXEL_SIZE;
	}
	length = (expected - player->x) / cos(angle);
	return (length);
}

/**
 * Lo mismo que get_x_length, pero en lugar de cos, uso sin
 */

// Calculates the length of the next distance from Y axis
int	get_y_length(Player *player, int distance, float angle)
{
	if (((int)(angle * 180 / PI)) == 0
		|| ((int)(angle * 180 / PI)) == 180)
	{
		return (0);
	}
	int length = 0;
	int expected = 0;
	if (sin(angle) > 0)
	{
		expected = (int)(roundf(player->y / PIXEL_SIZE + distance)) * PIXEL_SIZE + 1;
	}
	else
	{
		expected = (int)(roundf(player->y / PIXEL_SIZE - distance + 1)) * PIXEL_SIZE;
	}
	length = (expected - player->y) / sin(angle);
	return (length);
}

/**
 * Bug #1:
 * player.x = 457.4f
 * player.y = 448.4f 
 * player.angle = 276
 *
 * Fix #1:
 * Si length_y es 0, entonces asignar al siguiente punto
 *
 * Bug #2:
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
 * Fix #2:
 * He eliminado Fix #1 y lo he arreglado añadiendo la condición
 * length_y != get_y_length(player, i + 1), básicamente si el siguiente
 * punto con respecto al punto y es el mismo que el actual, se sigue quedando
 * con length_x, esto parece ser que arregla el bug inicial
 *
 * NOTA:
 * Le he añadido angle como parámetro para poder dibujar los puntos del fov,
 * además tiene que estar convertido en radianes
 * También he añadido color, para ver los diferentes puntos bien
 *
 * Bug #3:
 * Para angle = 270 no se calculan bien los puntos
 *
 * Fix #3:
 * El Fix #2 antes era Fix #3 porque no sé contar, ahora está bien, además
 * he corregido el Bug #3, simplemente he añadido otra condición en la que
 * length = length_y siguiendo la misma lógica que Fix #2, pero con respecto
 * al punto x
 *
 * Bug #4:
 * El siguiente punto se debería calcular con respecto a la nueva posición
 * del punto, porque de lo contrario se skippean puntos
 *
 * NOTA:
 * Me he dado cuenta que el Bug #4, técnicamente los puntos los calculo bien,
 * pero el problema es que al quedarme con una longitud, sí que calculo mal
 * los puntos, pero si pintase tanto length_x como length_y pintaría todos
 * los puntos correctamente.
 * Creo que me puedo quedar con las dos longitudes, y simplemente pintar
 * los que me renten
 *
 * NOTA:
 * Me acabo de dar cuenta que solo necesito conseguir 2 distancias con las
 * funciones get_length, y la diferencia entre estas es el factor por el
 * que tengo que multiplicarlas, sigo teniendo el mismo problema pero
 * reduzco el número de llamadas a get_*_length
 */
void	draw_n_points(Player *player, int n_points, float angle, Color color)
{
	int length_x1 = get_x_length(player, 1, angle);
	int length_y1 = get_y_length(player, 1, angle);

	int length_x2 = get_x_length(player, 2, angle);
	int length_y2 = get_y_length(player, 2, angle);

	int factor_x = length_x2 - length_x1;
	int factor_y = length_y2 - length_y1;
	for (int i = 0; i < n_points; ++i)
	{
		//int length_x = get_x_length(player, i + 1, angle);
		//int length_y = get_y_length(player, i + 1, angle);
		//int length = length_x; if ((length_y < length_x && length_y != get_y_length(player, i + 1, angle))
		//	|| length_x == get_x_length(player, i + 1, angle))
		//{
		//	length = length_y;
		//}
		//printf("Calculated length_x %d: %f %f\n",
		//		i,
		//		player->x + cos(angle) * length_x,
		//		player->y + sin(angle) * length_x);
		//printf("Calculated length_y %d: %f %f\n", i, player->x + cos(angle) * length_y, player->y + sin(angle) * length_y);
		//DrawCircle(player->x + cos(angle) * length, player->y + sin(angle) * length, 2, color);
		DrawCircle(player->x + cos(angle) * (length_x1 + factor_x * i), player->y + sin(angle) * (length_x1 + factor_x * i), 3, GetColor(0xFFAAAAAA));
		DrawCircle(player->x + cos(angle) * (length_y1 + factor_y * i), player->y + sin(angle) * (length_y1 + factor_y * i), 3, GetColor(0xAAAAFFAA));
	}
}

/**
 * NOTA:
 * Al crear esta función me he dado cuenta que no calculo todos los puntos
 * pegados al grid
 *
 * NOTA:
 * Creo que tengo que calcular la colisión del muro convirtiendo las
 * coordenadas reales a coordenadas escaladas, es decir, si en el mapa
 * hay un muro en (5, 5), entonces las coordenadas escaladas el muro
 * empezaría en el (5 * PIXEL_SIZE, 5 * PIXEL_SIZE)
 * y el muro tendría un tamaño de PIXEL_SIZE, entonces si el punto calculado
 * está en ese punto, habría colisión y tendría que quedarme con el punto
 * de colisión más cercano al jugador
 *
 * NOTA:
 * Para x
 * Si (angle > 90 && angle < 270)
 * Entonces el choque con el muro debería ser x - 1, porque el
 * jugador estaría mirando hacia la "izquierda" y el muro se lo
 * encontraría una posición antes
 *
 * Para y
 * Si (angle > 180 && angle < 360)
 * Entonces el choque con el muro debería ser y - 1, porque el
 * jugador estaría mirando hacia la "arriba" y el muro se lo
 * encontraría una posición antes
 *
 * Bug #1:
 * player.x = 86.0f
 * player.y = 42.0f
 * player.angle = 45
 */
Vector2	get_collided_point(Player *player, float angle, int map[ROWS][COLS])
{
	Vector2 p = {0};
	int length_x1 = get_x_length(player, 1, angle);
	int length_x2 = get_x_length(player, 2, angle);
	int factor_x = length_x2 - length_x1;

	int length_y1 = get_y_length(player, 1, angle);
	int length_y2 = get_y_length(player, 2, angle);
	int factor_y = length_y2 - length_y1;
	Vector2 px = {0};
	Vector2 py = {0};
	// Para la x
	for (int i = 0; i < 5; ++i)
	{
		px = (Vector2){
			player->x + cos(angle) * (length_x1 + factor_x * i),
			player->y + sin(angle) * (length_x1 + factor_x * i),
		};
		int x = px.x / PIXEL_SIZE;
		int y = px.y / PIXEL_SIZE;
		if (RADS_TO_DEG(angle) > 90 && RADS_TO_DEG(angle) < 270
			&& x - 1 >= 0 && x < ROWS
			&& y >= 0 && y < COLS
			&& map[x - 1][y] == 1)
		{
			break;
		}
		else if (x >= 0 && x < ROWS
			&& y >= 0 && y < COLS
			&& map[x][y] == 1)
		{
			break;
		}
	}
	// Para la y
	for (int i = 0; i < 100; ++i)
	{
		py = (Vector2){
			player->x + cos(angle) * (length_y1 + factor_y * i),
			player->y + sin(angle) * (length_y1 + factor_y * i),
		};
		int x = py.x / PIXEL_SIZE;
		int y = py.y / PIXEL_SIZE;
		if (RADS_TO_DEG(angle) > 180 && RADS_TO_DEG(angle) < 360
			&& x >= 0 && x < ROWS
			&& y - 1 >= 0 && y < COLS
			&& map[x][y - 1] == 1)
		{
			break;
		}
		else if (x >= 0 && x < ROWS
			&& y >= 0 && y < COLS
			&& map[x][y] == 1)
		{
			break;
		}
	}
	float new_x1 = fabs(px.x - player->x);
	float new_y1 = fabs(px.y - player->y);

	float new_x2 = fabs(py.x - player->x);
	float new_y2 = fabs(py.y - player->y);

	p = py;
	if (new_x1 * new_x1 + new_y1 * new_y1
		<= new_x2 * new_x2 + new_y2 * new_y2 || factor_y == 0)
	{
		if (factor_x != 0)
		{
			p = px;
		}
	}
	//DrawCircleV(px, 3, SKYBLUE);
	//DrawCircleV(py, 3, YELLOW);
	return (p);
}

int main(void)
{
	Player player = {
		.x = 86,
		.y = 42,
		.angle = 45,
		.size = 5,
		.fov = 100,
	};
	//Player player = {
	//	.x = 2 * PIXEL_SIZE,
	//	.y = 2 * PIXEL_SIZE,
	//	.angle = 45,
	//	.size = 4,
	//	.fov = 100,
	//};
	static int map[ROWS][COLS];
	for (int i = 0; i < 5; ++i)
	{
		map[0][i] = 1;
		map[4][i] = 1;
		map[i][0] = 1;
		map[i][4] = 1;
	}
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raycasting2");
	SetTargetFPS(60);
	int n_points = 10;
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

		// Draw map
		for (int i = 0; i < ROWS; ++i)
		{
			for (int j = 0; j < COLS; ++j)
			{
				if (map[i][j] == 1)
				{
					DrawRectangle(j * PIXEL_SIZE, i * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, GetColor(0xA0A030FF));
				}
			}
		}

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
		DrawLine(player.x, player.y, player.ray.x, player.ray.y, GREEN);

		//Vector2 lray = {
		//	.x = player.x + cos(DEG_TO_RADS(proper_mod(player.angle - 15, 360))) * player.fov,
		//	.y = player.y + sin(DEG_TO_RADS(proper_mod(player.angle - 15, 360))) * player.fov,
		//};
		//DrawLine(player.x, player.y, lray.x, lray.y, GetColor(0xFFFFFFFF));

		//Vector2 rray = {
		//	.x = player.x + cos(DEG_TO_RADS(proper_mod(player.angle + 15, 360))) * player.fov,
		//	.y = player.y + sin(DEG_TO_RADS(proper_mod(player.angle + 15, 360))) * player.fov,
		//};
		//DrawLine(player.x, player.y, rray.x, rray.y, GetColor(0xFFFFFFFF));

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

		//for (int i = 0; i < 15; ++i)
		//{
		//	draw_n_points(&player, n_points, DEG_TO_RADS(player.angle + i), SKYBLUE);
		//	draw_n_points(&player, n_points, DEG_TO_RADS(player.angle - i), SKYBLUE);
		//}
		//draw_n_points(&player, n_points, DEG_TO_RADS(player.angle), YELLOW);
		Vector2 p = get_collided_point(&player, DEG_TO_RADS(player.angle), map);
		DrawCircleV(p, 5, RAYWHITE);
		EndDrawing();
	}
	CloseWindow();
	return (0);
}
