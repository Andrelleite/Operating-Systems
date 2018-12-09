Main: drone_movement.c semlib.c queue.c Main.c
	gcc -Wall -pthread drone_movement.c semlib.c queue.c Main.c -lm -o Main