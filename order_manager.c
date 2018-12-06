#include "header.h"
#include "drone_movement.h"
#include "semlib.h"

int main()
{

  char sender[1024];
  char order[1024];
  char *pos;
  // Opens the pipe for writing
  int fifo_id;

  printf("___ORDER MANAGER___\n\n");
  printf("[Order]: prod: product_name, quantity to: x, y\n");

  if((fifo_id = open(FIFO, O_WRONLY)) < 0){
    perror("Error opening pipe for reading");
    exit(-1);
  }
  // Do some work
  while (1) {
    printf("[Order]: ");
    fgets(sender,1024,stdin);
    sprintf(order,"ORDER %d ",order_id);
    if ((pos=strchr(sender, '\n')) != NULL){
      *pos = '\0';
    }
    strcat(order,sender);
    write(fifo_id, order, 1024*sizeof(char));
    memset(&sender[0], 0, sizeof(sender));
    memset(&order[0], 0, sizeof(order));
    sleep(10);
  }

}
