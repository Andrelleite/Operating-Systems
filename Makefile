Main: drone_movement.c semlib.c Main.c
	gcc -Wall -pthread Main drone_movement.c semlib.c Main.c -lm -o Main