#CFLAGS := -Wall -Wextra -Werror
CPPFLAGS := -I ./include/MLX42/include/MLX42 -I ./include/BFL/include
LDFLAGS := -L ./include/BFL -L ./include/MLX42/build/
LDLIBS := -lBFL -lmlx42 -lglfw -pthread -lm -ldl

raycasting: bfl mlx
	clang -o ./raycasting/raycasting $(CFLAGS) -g ./raycasting/main.c $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)
	./raycasting/raycasting

animation: bfl mlx
	clang -o ./animation/animation $(CFLAGS) -g ./animation/main.c $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)
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
	make fclean -C ./include/BFL
	make clean -C ./include/MLX42/build

.PHONY: animation raycasting bfl mlx clean
