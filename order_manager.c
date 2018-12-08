#include "header.h"
#include "drone_movement.h"
#include "semlib.h"

int main()
{

  char order[1024];
  char *pos;
  // Opens the pipe for writing
  int fifo_id;

  printf("___ORDER MANAGER___\n\n");
  printf("[Order]: prod: product_name, quantity to: x, y\n");
  printf("[Add Drone]: Drones y\n\n");

  if((fifo_id = open(FIFO, O_WRONLY)) < 0){
    perror("Error opening pipe for reading");
    exit(-1);
  }
  // Do some work
  while (1) {
    printf("[COMANDO]: ");
    fgets(order,1024,stdin);
    if ((pos=strchr(order, '\n')) != NULL){
      *pos = '\0';
    }
    write(fifo_id, order, 1024*sizeof(char));
    memset(order,0,1024);
    sleep(5);
  }

}
