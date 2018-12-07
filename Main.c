/************************************************************************************
*
*		META 2 - SISTEMAS OPERATIVOS , 2018-2019
*
*		AUTORES: André Leite e Sara Inácio
*		DATA: 04/12/2018
*		LANGUAGE: C
*		COMPILE AS: gcc -Wall -pthread drone_movement.c semlib.c queue.c Main.c -lm -o Main
*
*************************************************************************************/

#include "header.h"
#include "queue.h"
#include "drone_movement.h"
#include "semlib.h"

q_node list_orders;
Statistic *mem;
Drone *drones;
Warehouse *warehouses;
Warehouse *ware_mem;

int shutdown =0;
int fifo_id;
int msgq_id;
int sem;
int order_id;
char logInfo[256];

pthread_t *drone_idp;
pthread_mutex_t mutex =  PTHREAD_MUTEX_INITIALIZER;

void terminate(int sign){

	int i = 0;
	shutdown = 1;
	printf("\nCleaning up memory.\nEnding program.\n");

	for(i = 0; i < WAREHOUSES; i++){
		wait(&ware_mem[i].id);
	}
	wait(&central);
	printf("\nCleaning memory.\n");
	free(drone_idp);
	free(drones);

	printf("\nCleaning statistics shared memory.\n");
	if(shmctl(shmid_stat,IPC_RMID,NULL) < 0){
		perror("Error cleaning shared memory\n");
	}
	printf("\nCleaning warehouse shared memory.\n");
	if(shmctl(shmid_ware,IPC_RMID,NULL) < 0){
		perror("Error cleaning shared memory\n");
	}
	printf("\nClosing message queue.\n");
	msgctl(msgq_id,IPC_RMID,NULL);


	printf("\nMemory has been cleaned.\n");
	printf("\nClosing sempahore.\n");

	sem_close(sem);
	close(fifo_id);
	unlink(FIFO);
	pthread_mutex_destroy(&mutex);
	pthread_exit(0);
	exit(0);

}

void insert_log(char *text){
	FILE *f;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char *find = strchr(text, '-');
	signal(SIGINT,SIG_IGN);
	sem_wait(sem, 0);

	f = fopen("output.log", "a+");

	if(find != NULL)
	{
		fprintf(f, "%s %d:%d:%d ------------\n\n",text,  tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
	else
	{
		fprintf(f, "%d:%d:%d %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, text);
	}

	fclose(f);
	sem_post(sem, 0);
	signal(SIGINT,terminate);

}

void init_shmem(){

	//Create shared memory segment
	if((shmid_ware = shmget( key1 ,WAREHOUSES*sizeof(warehouses),IPC_CREAT|0777)) == -1){
		perror("Error creating shared memory");
		exit(-1);
	}
	if((shmid_stat = shmget( key2 ,sizeof(Statistic),IPC_CREAT|0777)) == -1){
		perror("Error creating shared memory");
		exit(-1);
	}

	//Map shared memory to process address
	mem = (Statistic *)shmat(shmid_stat,NULL,0);
	ware_mem = (Warehouse *)shmat(shmid_ware,NULL,0);

}

void read_config(char *file){

	FILE *f = fopen(file,"r");
	char line[1024] = "";
	char *word;
	char *pos;
	int i;
	int w = 0;
	int par = 0;
	int lines = 0;
	int tokens = 0;

	// check if file exists
	if(file == NULL){
		perror("File is not existent\n");
		exit(-1);
	}

	while(fgets(line,1024,f)!=NULL){

		lines++;
		if(lines == 1){
			word = strtok(line,",");
			while(word != NULL){
				if(tokens == 0){
					MAX_X = atoi(word);
				}else if(tokens == 1){
					MAX_Y = atoi(word);
				}
				tokens++;
				word = strtok(NULL,", \n\0");
			}
			printf("MAX_X : %d\nMAX_Y : %d\n",MAX_X,MAX_Y);
		}else if(lines == 2){
			word = strtok(line,",");
			while(word != NULL){
				if ((pos=strchr(word, '\n')) != NULL){
		      *pos = '\0';
		    }
				all_products[tokens] = (char *)malloc(strlen(word)*sizeof(char));
				strcpy(all_products[tokens],word);
				tokens++;
				word = strtok(NULL,",");
			}
			PRODUCTS_NUMBER = tokens;
			for(i = 0; i < tokens; i++){
				printf("Product name: %s\n",all_products[i]);
			}
		}else if(lines == 3){
			word = strtok(line,",");
			while(word != NULL){
				DRONES = atoi(word);
				word = strtok(NULL,",");
			}
			printf("Number of Drones: %d\n",DRONES);
		}else if(lines == 4){
			word = strtok(line,",");
			while(word != NULL){
				if(tokens == 0){
					FILL_FREQ = atoi(word);
				}else if(tokens == 1){
					QUANTITY = atoi(word);
				}else if(tokens == 2){
					TIME = atoi(word);
				}
				tokens++;
				word = strtok(NULL,",");
			}
			printf("Fill frequency: %d\nQuantity of items: %d\nTime unit: %d\n",FILL_FREQ,QUANTITY,TIME);
		}else if(lines == 6){
			word = strtok(line,"\n");
			WAREHOUSES = atoi(word);
			printf("Number of warehouses: %d\n",WAREHOUSES);
			init_shmem();
		}else if(lines == 7){
			i = 0;

			do{
				par = 0;
				w = 0;
				word = strtok(line," ");
				ware_mem[i].name = (char *)malloc(strlen(word)*sizeof(char));
				strcpy(ware_mem[i].name,word);
				while(word != NULL){
					if(tokens == 1){
						ware_mem[i].coord_x = atoi(word);
					}else if(tokens == 2){
						ware_mem[i].coord_y = atoi(word);
					}else if(tokens == 4){
						tokens--;
						par++;
						if(par % 2 != 0){
							ware_mem[i].product[w].name = (char *)malloc(strlen(word)*sizeof(char));
							strcpy(ware_mem[i].product[w].name,word);
						}else{
							ware_mem[i].product[w].quantity = atoi(word);
							w++;
						}
					}
					ware_mem[i].n_products = w;
					word = strtok(NULL,"xy: ,");
					tokens++;
				}
				printf("ADEIOS");
				i++;
				tokens = 0;
			}while(fgets(line,1024,f)!=NULL && i < WAREHOUSES);
		}
		tokens = 0;
	}
	if(lines == 0){
		printf("File is empty\n");
	}
	fclose(f);

}

void initialize_stats(Statistic *stat){

	stat->travels_made = 0;
	stat->total_encomendas = 0;
	stat->shipped_prods = 0;
	stat->loaded_prods = 0;
	stat->delivered_prods = 0;
	stat->derivered_requests = 0;
	stat->restocks = 0;

}

void update_stock(int n){

	Message message;
	int i;
	int load_var;
	int units;
	int product_id;

	while(1){
		msgrcv(msgq_id,&message,sizeof(message),n,0);
		load_var = 0;
		if(message.type == 1){
			i = message.ware_id;
			units = message.quantity_prod;
			product_id = message.prod_id;
			if(ware_mem[i].product[product_id].quantity < MAX_STOCK){
				if(MAX_STOCK - ware_mem[i].product[product_id].quantity > units){
					ware_mem[i].product[product_id].quantity += units;
					sprintf(logInfo, "Stock do produto %s atualizado em %d unidades, no Armazem %d", ware_mem[i].product[product_id].name, units, ware_mem[i].id);
					printf("Warehouse %s received %d of %s\n",ware_mem[i].name,units,ware_mem[i].product[product_id].name);
					insert_log(logInfo);
				}else{
					int s = MAX_STOCK - ware_mem[i].product[product_id].quantity;
					ware_mem[i].product[product_id].quantity += MAX_STOCK - ware_mem[i].product[product_id].quantity;
					sprintf(logInfo, "Stock do produto %s atualizado em %d unidades, no Armazem %d", ware_mem[i].product[product_id].name, s, ware_mem[i].id);
					printf("Warehouse %s received %d of %s\n",ware_mem[i].name,units,ware_mem[i].product[product_id].name);
					insert_log(logInfo);
				}
				mem->restocks++;
			}else{
				sprintf(logInfo, "Stock do produto %s atingiu o limite no Armazem %d", ware_mem[i].product[product_id].name, ware_mem[i].id);
				printf("Stock of product %s in warehouse %s has reached the maximum.\n",ware_mem[i].product[product_id].name, ware_mem[i].name);
				insert_log(logInfo);
			}
			CHOOSE_WARE++;
		}
		else{
			printf("PROCESSING ORDER %d FOR DRONE %d\n",message.order_id - WAREHOUSES, message.drone_id );
			while(load_var < message.quantity_prod){
				load_var++;
				sleep(1);
			}
			message.mtype = message.order_id;
			if(msgsnd(msgq_id,&message,sizeof(message),0) < 0){
				perror("Error sending message.\n");
				exit(-1);
			}
		}
	}
	//printf("\n\n----------------------------\n");
}

void init_pipe(){

	// Create named pipe
	if((mkfifo(FIFO, O_CREAT|O_EXCL|0600) < 0) && (errno != EEXIST)){
		perror("Error creating pipe");
		exit(-1);
	}

	if((fifo_id = open(FIFO, O_RDONLY|O_NONBLOCK)) < 0){
		perror("Error opening pipe for reading");
		exit(-1);
	}


}

void init(){

	sem = sem_get(1,1);

	insert_log("Programa Iniciado");
	read_config("config.txt");
	drone_idp = (pthread_t*)malloc(DRONES * sizeof(pthread_t));
	drones = (Drone*)malloc(DRONES * sizeof(Drone));

	//Create message queue

	if((msgq_id = msgget(message_key,IPC_CREAT|0700)) < 0){
		perror("Error creating message pipe");
		exit(-1);
	}


	initialize_stats(mem);

}

void show_stats(int signum){

	printf("\nTotal travels: %d\n",mem->travels_made);
	printf("Total de Encomendas: %d\n",mem->total_encomendas);
	printf("Produtos enviados: %d\n",mem->shipped_prods);
	printf("Produtos preparados: %d\n",mem->loaded_prods);
	printf("Produtos entregues: %d\n",mem->delivered_prods);
	printf("Total pedidos: %d\n",mem->derivered_requests);
	printf("Total restocks: %d\n\n",mem->restocks);
	signal(SIGUSR1,show_stats);
}

void print_warehouse_info(int n){

	int i = 0;
	Warehouse w = ware_mem[n];
	printf("Name: %s\nID: %d\nCoordinate x: %.2f\nCoordinate y: %.2f\n\n",w.name,w.id,w.coord_x,w.coord_y);
	printf("Has products: \n");
	while(i < w.n_products){
		printf("Product: %s | Quantity: %d\n",w.product[i].name,w.product[i].quantity);
		i++;
	}

}

void create_warehouses(){

	int n = 0;
	signal(SIGUSR1,show_stats);

	for(n = 0; n < WAREHOUSES; n++){

		if(fork() == 0){
			ware_mem[n].id = getpid();
			ware_mem[n].idpr = n+1;
			sprintf(logInfo, "Criação Processo Armazem %d com o PID %d", n, ware_mem[n].id);
			printf("Criação Processo Armazem %d com o PID %d\n", n, ware_mem[n].id);
			insert_log(logInfo);
			update_stock(n+1);;
		}
	}

}

void drone_travel(Drone *drone, double x, double y){

	int result = -1;
	// move drones to warehouse
	sprintf(logInfo, "Drone %d a iniciar movimento até ao armazém", drone->idpr);
	insert_log(logInfo);
	printf("\n[%d]DRONE(%.2f,%.2f)",drone->idpr,drone->coord_x,drone->coord_y);
	printf(" to (%.2f,%.2f)\n",x,y);
	while(result != 0){
		result = move_towards(&(drone->coord_x),&(drone->coord_y),x,y);
		if(result == 0){
			printf("Drone %d moved correctly toward warehouse.\n",drone->idpr);
			sprintf(logInfo, "Drone %d movimentado com sucesso até ao armazém", drone->idpr);
			insert_log(logInfo);
			mem->travels_made += 1;
			printf("Drone final position (%.2f , %.2f)\n\n",drone->coord_x,drone->coord_y);
		}else if(result == -1){
			printf("Drone %d was already in site.\n",drone->idpr);
			sprintf(logInfo, "Drone %d já localizado no armazém", drone->idpr);
			insert_log(logInfo);
		}else if(result == -2){
			printf("Trajectory turned an error.\n");
			sprintf(logInfo, "Drone %d não movimentado até ao armazém, erro na trajetória", drone->idpr);
			insert_log(logInfo);
		}
	}
	sprintf(logInfo, "Drone %d localizado na posição (%.2f , %.2f)", drone->idpr, drone->coord_x,drone->coord_y);
	insert_log(logInfo);
}

void *do_some_work(void *id){

	int idt = *((int *)id);
	Message message;
	Drone *drone = (Drone *)malloc(sizeof(Drone));
	drone = &drones[idt-1];
	printf("DRONE %d WAITING\n",idt );

	while(shutdown != 1){


		if(drone->state != 0){
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex);

			/* send to warehouse*/

			drone_travel(drone,drone->warehouse_s->coord_x,drone->warehouse_s->coord_y);

			/* load products , send message to warehouse*/
			message.type = 2;
			message.mtype = drone->warehouse_s->idpr;
			message.drone_id = idt;
			message.order_id = WAREHOUSES + drone->order_id;
			message.quantity_prod = drone->quantity;
			printf("LOADING DRONE %d\n",drone->idpr);
			printf("DRONE %d REQUEST %d\n",drone->idpr, drone->quantity);
			if(msgsnd(msgq_id,&message,sizeof(message),0) < 0){
				perror("Error sending message.\n");
				exit(-1);
			}
			msgrcv(msgq_id,&message,sizeof(message),message.order_id,0);
			printf("WAREHOUSE FINISHED LOADING PRODUCTS\n");
			/* send to delivery checkpoint */
			printf("to destiny \n");
			drone_travel(drone,drone->destiny_x,drone->destiny_y);

			/* send back to base*/
			printf("Back to base %d\n",drone->base_no );
			drone_travel(drone,bases[drone->base_no][0],bases[drone->base_no][1]);

			drone->order_id = 0;
			drone->state = 0;
		}

	}

	pthread_exit(0);
	exit(0);
}

void create_drones(){

	int i = 0;
	int j = 0;
	int err;
	time_t t;
	srand((unsigned)time(&t));
	pthread_mutex_lock(&mutex);

	while(i != DRONES){
		sleep(1);
		drones[i].idpr = i+1;
		drones[i].coord_x = bases[j][0];
		drones[i].coord_y = bases[j][1];
		drones[i].state = 0; /*0  -> free*/
		drones[i].base_no = j;
		drones[i].loaded = 0;
		if((err = pthread_create(&drones[i].id, NULL, &do_some_work,&drones[i].idpr)) == 0){
			// print to log success creation
			sprintf(logInfo, "Drone %d criado", drones[i].idpr);
			insert_log(logInfo);
			// will make it move towards the warehouse
		}else{
			printf("thread failed to create.\n");
			exit(-1);
		}
		j = (j+1)%4;
		i++;
	}

	sleep(2);
	printf("\n");

}

void wait_for_signal(){
	//printf("\n------- STOCK UPDATE -------\n");
	Message message;
	message.type = 1;

	while(1){
		sleep(FILL_FREQ);
		message.ware_id = CHOOSE_WARE % WAREHOUSES;
		message.mtype = (CHOOSE_WARE % WAREHOUSES) +1;
		message.quantity_prod = QUANTITY;
		message.prod_id = rand() % ware_mem[message.ware_id].n_products;
		if(msgsnd(msgq_id,&message,sizeof(message),0) < 0){
			perror("Error sending message.\n");
			exit(-1);
		}
		CHOOSE_WARE++;
	}
}

int check_if_int(char dig[]){

	int value = atoi(dig);
	int ret = 1;
	if(value == 0){
		ret = 0;
	}else{
		ret = 1;
	}
	return ret;

}

int check_prod_exists(char prod[]){

	int i = 0;
	int flag = 0;
	while(i < PRODUCTS_NUMBER && flag == 0){
		if(strcmp(prod, all_products[i]) == 0){
			flag = 1;
		}
		i++;
	}
	return flag;
}

void reserver(Warehouse *w_to, char *p_name, int res){

	int i;
	int j;
	for(i = 0; i < WAREHOUSES; i++){
		if(strcmp(ware_mem[i].name,w_to->name)==0){
			for(j = 0; j < ware_mem[i].n_products; j++){
				if(strcmp(ware_mem[i].product[j].name,p_name)==0){
					ware_mem[i].product[j].quantity -= res;
				}
			}
		}
	}
}

void select_poles(){

	int i;
	int j;
	int w = 0;
	int d = 0;
	double min_distance;
	double dist = 0;
	double x = (double)list_orders->next->order->x;
	double y = (double)list_orders->next->order->y;

	Warehouse capable[WAREHOUSES];
	Drone *send_drone = (Drone *)malloc(sizeof(Drone));
	Warehouse *receiver_ware = (Warehouse *)malloc(sizeof(Warehouse));

	for(i = 0; i < WAREHOUSES; i++){
		for(j = 0; j < ware_mem[i].n_products ; j++){
			if(strcmp(list_orders->next->order->prod_name,ware_mem[i].product[j].name) == 0){
				if(ware_mem[i].product[j].quantity >= list_orders->next->order->quantity){
					capable[w] = ware_mem[i];
					w++;
				}
			}
		}
	}
	printf("%d warehouses with needed resources\n",w);
	if(w > 0){
		for(i = 0; i < DRONES; i++){
			if(drones[i].state == 0){
				for(j = 0; j < w; j++){
					if(d==0){
						min_distance = distance(drones[i].coord_x,drones[i].coord_y,capable[j].coord_x,capable[j].coord_y);
						min_distance += distance(capable[j].coord_x,capable[j].coord_y,x,y);
						receiver_ware = &capable[j];
						send_drone = &drones[i];
						printf("New minimum distance : %.2f\n",min_distance);
						d++;
					}else{
						dist = distance(drones[i].coord_x,drones[i].coord_y,capable[j].coord_x,capable[j].coord_y);
						dist += distance(capable[j].coord_x,capable[j].coord_y,x,y);
						if(dist < min_distance){
							receiver_ware = &capable[j];
							send_drone = &drones[i];
							min_distance = dist;
							printf("New minimum distance : %.2f\n",min_distance);
						}
						d++;
					}
				}
			}
		}
		reserver(receiver_ware,list_orders->next->order->prod_name,list_orders->next->order->quantity);
		send_drone->warehouse_s = (Warehouse *)malloc(sizeof(Warehouse));
		send_drone->warehouse_s = receiver_ware;
		send_drone->order_id = list_orders->next->order->id;
		send_drone->destiny_x = list_orders->next->order->x;
		send_drone->destiny_y = list_orders->next->order->y;
		send_drone->state = 1;
		send_drone->quantity = list_orders->next->order->quantity;
		pop_node(list_orders->next,list_orders);
		printf("DEALER | Drone: %d to: %s , state %d order %d\n",send_drone->idpr, send_drone->warehouse_s->name, send_drone->state, send_drone->order_id);
	}else{
		delay_order(list_orders->next->order->id, list_orders);
	}

}

void approve_order(char order[]){

	Order *new_order = (Order *)malloc(sizeof(Order));
	char *word;
	int check = 0;
	int p_exists = 0;
	int i = 0;
	int lenght;
	char key_words[20][256];
	printf("[CENTRAL] Received new order (%s)\n",order);

	if(order != NULL && strlen(order) > 13){
		word = strtok(order," ,");
		while(word != NULL){
			strcpy(key_words[i],word);
			if(check_if_int(word)){
				check++;
			}
			if(i == 3){
				p_exists = check_prod_exists(word);
			}
			word = strtok(NULL," ,");
			i++;
		}
		lenght = i;
		if(lenght == 8 && p_exists && check == 4){
			new_order->id = order_id++;
			new_order->prod_name = (char *)malloc(strlen(key_words[3])*sizeof(char));
			strcpy(new_order->prod_name,key_words[3]);
			new_order->quantity = atoi(key_words[4]);
			new_order->x = atoi(key_words[6]);
			new_order->y = atoi(key_words[7]);
			printf("REQUEST ACCEPTED.\n");
			insert_q(new_order,list_orders);
			print_q(list_orders);
		}else{
			printf("REQUEST WAS REJECTED\n");
		}
	}
	if(list_orders->next != NULL){
		select_poles();
	}

}

void read_request(){

	char order[1024];
	printf("Listenning to pipe\n");
	while (1) {
    read(fifo_id, &order, 1204*sizeof(char));
		approve_order(order);
		sleep(20);
  }

}

int main(){

	list_orders = init_q();
	init();
	insert_log("\n\n------------ INICIO LOG");
	signal(SIGINT,terminate);
	printf("\n[%d] SIMULATION PROCESS\n",getpid());
	create_warehouses();
	// create Central process
	if((central = fork()) == 0){
		sleep(WAREHOUSES);
		printf("\n[%d] CENTRAL\n",getpid());
		init_pipe();
		create_drones();
		read_request();
	}else{
		kill(getpid(),SIGUSR1);
		wait_for_signal();
	}

}
