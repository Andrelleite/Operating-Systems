/************************************************************************************
*
*		META 2 - SISTEMAS OPERATIVOS , 2018-2019
*
*		AUTORES: André Leite e Sara Inácio
*		DATA: 06/11/2018
*		LANGUAGE: C
*		COMPILE AS: no compile
*		FILE: header.c
*
*************************************************************************************/
#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "semlib.h"


#define key1 1243
#define key2 2341
#define message_key 2000
#define FIFO "NAMED_PIPE"
#define MAX_STOCK 100



typedef struct product *lista_prods;

typedef struct product{

	int quantity;
	char *name;

}Product;


typedef struct warehouse{

	pid_t id;
	int idpr;
	int n_products;
	char *name;
	double coord_x;
	double coord_y;
	Product product[128];

}Warehouse;

typedef struct drone{

	Warehouse *warehouse_s;
	pthread_t id;
	char *product;
	int idpr;
	int order_id; // 0 se nao tem pedidos
	double coord_x;
	double coord_y;
	double destiny_x;
	double destiny_y;
	int quantity;
	int base_no;
	int state;
	int loaded;

}Drone;

typedef struct message{

	long mtype;
	int type;
	int quantity_prod;
	int ware_id;
	int prod_id;
	Drone *drone;
	int load_done;

}Message;

typedef struct statistics{

	int travels_made;
	int total_encomendas;
	int shipped_prods;
	int loaded_prods;
	int delivered_prods;
	int derivered_requests;
	int restocks;

}Statistic;


int MAX_X; // COMPRIMENTO MAXIMO
int MAX_Y; // ALTURA MAXIMA
int WAREHOUSES = 1; // NUMERO DE ARMAZENS
int DRONES = 1; // NUMERO DE DRONES
int FILL_FREQ = 1; // FREQUENCIA DE ABASTECIMENTO
int QUANTITY = 1; // QUANTIDADE DE ABASTECIMENTO
int TIME = 1; // UNIDADE DE TEMPO (SEGUNDOS)
int CHOOSE_WARE = 0; //WAREHOUSE TO FILL
int PRODUCTS_NUMBER = 0; //QUANTIDADE DE PRODUTOS

int bases[4][2] = {{0,0},{0,1080},{1920,1080},{1920,0}};
int order_id = 1;
char *all_products[128];
int shmid_stat;
int shmid_ware;
char *pip;
pid_t central;



#endif
