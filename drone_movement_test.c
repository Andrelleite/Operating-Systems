/*C**********************************************************************
* FILENAME :   	drone_movement_test.c
* COMPILE AS: gcc -Wall drone_movement_test.c drone_movement.c -lm -o so2018
*
* DESCRIPTION :
*       		Support code for the Operating Systems (SO)
*				Practical Assignment 2018.
*
* Test file to work with functions provided by drone_movement.c
*
* AUTHOR :  	Nuno Antunes
* DATE   :  	2018/10/14
*
*C*/

#include "drone_movement.h"

int test_case (double ax, double ay, double t_x, double t_y, int max_attempts);


int main(){


	double ax = 0;
	double ay = 0;

	double t_x = 100;
	double t_y = 100;


	// simple test, diagonal movement
	test_case (ax,  ay,  t_x, t_y, 145);

	// reverse simple
	test_case (t_x, t_y,  ax,  ay, 145);

	// rectangle 1x2
	test_case (0,  0,  100, 200, 235);

	// rectangle 1x2.xx
	test_case (0,  0,  120, 343, 370);

	// back and up
	test_case (500,  0,  120, 343, 1000);

	// back and down
	test_case (500,  500,  120, 343, 1000);

	// front and down
	test_case (0,  500,  120, 343, 1000);

	// back to the base
	test_case (500,  500,  0, 0, 1000);



	// simple invalid tests
	test_case (-1,  1,  t_x, t_y, 145);
	test_case (1,  -1,  t_x, t_y, 145);
	test_case (ax,  ay,  -t_x, t_y, 145);
	test_case (ax,  ay,  t_x, -t_y, 145);

	// Already on target test
	test_case (ax,  ay,  ax,  ay, 145);


	return 0;
}


int test_case (double ax, double ay, double t_x, double t_y, int max_attempts){
	printf("\n===========================================================================\n");
	printf("Start: (%f, %f) Target: (%f, %f) \n====================\n", ax, ay, t_x, t_y);

	int i, result;
	for(i = 0; i< max_attempts; i++){
		result = move_towards(&ax, &ay, t_x, t_y);

		printf("\t %d, %d, (%f, %f) \n", i, result, ax, ay);

		if(result == 0){
			printf("BINGO: %f == %f   %f == %f \n-------------------------\n\n", ax, t_x, ay, t_y);
			return i;
		} else if(result < 1){
			printf("ERROR %d:  %f == %f   %f == %f \n-------------------------\n\n", result, ax, t_x, ay, t_y);
			return i;
		}
	}
	return i;
}
