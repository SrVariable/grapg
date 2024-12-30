#CFLAGS := -Wall -Wextra -Werror
CPPFLAGS := -I ./include/MLX42/include/MLX42 -I ./include/BFL/include -I ./include/raylib-5.5_linux_amd64/include
LDFLAGS := -L ./include/BFL -L ./include/MLX42/build/ -L ./include/raylib-5.5_linux_amd64/lib
LDMLX := -lBFL -lmlx42 -lglfw -pthread -lm -ldl
LDRAYLIB := -lBFL -lglfw -pthread -lm -ldl -l:libraylib.a

raycasting2: bfl raylib
	clang -o ./raycasting2/raycasting $(CFLAGS) -g ./raycasting2/main.c $(CPPFLAGS) $(LDFLAGS) $(LDRAYLIB)
	./raycasting2/raycasting

raycasting: bfl mlx
	clang -o ./raycasting/raycasting $(CFLAGS) -g ./raycasting/main.c $(CPPFLAGS) $(LDFLAGS) $(LDMLX)
	./raycasting/raycasting

animation: bfl mlx
	clang -o ./animation/animation $(CFLAGS) -g ./animation/main.c $(CPPFLAGS) $(LDFLAGS) $(LDMLX)
	./animation/animation

bfl:
	git submodule update --init --recursive
	make -j4 -C ./include/BFL

mlx:
	git submodule update --init --recursive
	cmake -S include/MLX42 -B include/MLX42/build
	cmake --build include/MLX42/build -j4

clean:
	rm -f animation/animation
	rm -f raycasting/raycasting
	rm -f raycasting2/raycasting
	make fclean -C ./include/BFL
	make clean -C ./include/MLX42/build

.PHONY: build animation raycasting raycasting2 bfl mlx clean raylib
