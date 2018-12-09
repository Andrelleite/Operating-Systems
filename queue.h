/************************************************************************************
*
*		META 2 - SISTEMAS OPERATIVOS , 2018-2019
*
*		AUTORES: André Leite e Sara Inácio
*		DATA: 06/11/2018
*		LANGUAGE: C
*		COMPILE AS: no compile
*		FILE: queue.h
*
*************************************************************************************/

#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

typedef struct queue *q_node;

typedef struct order
{
	int id;
	char *prod_name;
	int quantity;
	int x;
	int y;

}Order;

typedef struct queue
{
    int n;
    int taken;
    Order *order;
    q_node next;

}Queue;

q_node init_q();

void insert_q(Order *new_node,q_node lista );

void print_q(q_node lista);

void pop_node(q_node node, q_node lista);

q_node search_order(int id, q_node lista);

void delay_order(int id, q_node lista);

#endif
