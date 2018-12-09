/************************************************************************************
*
*		META 2 - SISTEMAS OPERATIVOS , 2018-2019
*
*		AUTORES: André Leite e Sara Inácio
*		DATA: 09/12/2018
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

int shutdown = 0;
int fifo_id;
int msgq_id;
int sem;
int order_id;
char logInfo[256];

pthread_t *drone_idp;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

//realiza a terminação controlada do programa, ao receber o sinal SIGINT
void terminate(int sign)
{
	int i = 0;
	shutdown = 1;

	printf("\nCleaning up memory.\nEnding program.\n");
	signal(SIGINT,terminate);
	for(i = 0; i < DRONES; i++){
		drones[i].state = 0;
	}
	pthread_cond_broadcast(&cv);

	for(i = 0; i < WAREHOUSES; i++)
	{
		wait(&ware_mem[i].id);

		//atualiza ficheiro de log
		sprintf(logInfo, "Warehouse process %d with PID %d terminated", i, ware_mem[i].id);
		insert_log(logInfo);
	}

	wait(&central);
	printf("\nCleaning memory.\n");
	free(drone_idp);
	free(drones);

	printf("\nCleaning statistics shared memory.\n");
	if(shmctl(shmid_stat,IPC_RMID,NULL) < 0)
	{
		perror("Error cleaning shared memory\n");
	}

	printf("\nCleaning warehouse shared memory.\n");
	if(shmctl(shmid_ware,IPC_RMID,NULL) < 0)
	{
		perror("Error cleaning shared memory\n");
	}

	printf("\nClosing message queue.\n");
	msgctl(msgq_id,IPC_RMID,NULL);

	printf("\nMemory has been cleaned.\n");
	printf("\nClosing sempahore.\n");

	close(fifo_id);
	unlink(FIFO);
	sem_close(sem);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cv);
	pthread_exit(0);
	exit(0);
}

//insere a informação no ficheiro de log
void insert_log(char *text)
{
	FILE *f;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char *find = strchr(text, '-');

	sigprocmask (SIG_BLOCK, &block_ctrlc, NULL);
	sem_wait(sem, 0);

	//se o ficheiro não existir, cria um novo
	//se o ficheiro existir, faz append da informção
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
	sigprocmask (SIG_UNBLOCK, &block_ctrlc, NULL);
}

//inicializa a memória partilhada
void init_shmem()
{
	//cria segmento de memória partilhada
	if((shmid_ware = shmget( key1 ,WAREHOUSES*sizeof(warehouses),IPC_CREAT|0777)) == -1)
	{
		perror("Error creating shared memory");
		exit(-1);
	}

	if((shmid_stat = shmget( key2 ,sizeof(Statistic),IPC_CREAT|0777)) == -1)
	{
		perror("Error creating shared memory");
		exit(-1);
	}

	//mapeia a memória partilhada para o endereço do processo
	mem = (Statistic *)shmat(shmid_stat,NULL,0);
	ware_mem = (Warehouse *)shmat(shmid_ware,NULL,0);
}

//lê as configurações do ficheiro config.txt
void read_config(char *file)
{
	FILE *f = fopen(file,"r");
	char line[1024] = "";
	char *word;
	char *pos;
	int i;
	int w = 0;
	int par = 0;
	int lines = 0;
	int tokens = 0;

	//verifica se o ficheiro existe
	if(file == NULL)
	{
		perror("File is not existent\n");
		exit(-1);
	}

	//percorre as linhas do ficheiro
	while(fgets(line,1024,f)!=NULL)
	{
		lines++;

		//na primeira linha são definidos os limites da área
		if(lines == 1)
		{
			word = strtok(line,",");
			while(word != NULL)
			{
				//o primeiro valor é o limite de x
				if(tokens == 0)
				{
					MAX_X = atoi(word);
				}
				//o segundo valor é o limite de y
				else if(tokens == 1)
				{
					MAX_Y = atoi(word);
				}

				tokens++;
				word = strtok(NULL,", \n\0");
			}

			printf("MAX_X : %d\nMAX_Y : %d\n",MAX_X,MAX_Y);
		}
		//na segunda linha são definidos os produtos
		else if(lines == 2)
		{
			word = strtok(line,",");

			while(word != NULL)
			{
				if ((pos=strchr(word, '\n')) != NULL)
				{
		      		*pos = '\0';
		    	}

				all_products[tokens] = (char *)malloc(strlen(word)*sizeof(char));
				strcpy(all_products[tokens],word);
				tokens++;
				word = strtok(NULL,",");
			}

			PRODUCTS_NUMBER = tokens;

			for(i = 0; i < tokens; i++)
			{
				printf("Product name: %s\n",all_products[i]);
			}
		}
		//na terceira linha é definido o número de threads drone
		else if(lines == 3)
		{
			word = strtok(line,",");

			while(word != NULL)
			{
				DRONES = atoi(word);
				word = strtok(NULL,",");
			}

			printf("Number of Drones: %d\n",DRONES);
		}
		//na quarta linha são definidos a frequência de preenchimento, a quantidade de itens e a unidade de tempo
		else if(lines == 4)
		{
			word = strtok(line,",");

			while(word != NULL)
			{
				if(tokens == 0)
				{
					FILL_FREQ = atoi(word);
				}
				else if(tokens == 1)
				{
					QUANTITY = atoi(word);
				}
				else if(tokens == 2)
				{
					TIME = atoi(word);
				}

				tokens++;
				word = strtok(NULL,",");
			}

			printf("Fill frequency: %d\nQuantity of items: %d\nTime unit: %d\n",FILL_FREQ,QUANTITY,TIME);
		}
		//na sexta linha é definido o número de processos armazém
		else if(lines == 6)
		{
			word = strtok(line,"\n");
			WAREHOUSES = atoi(word);
			printf("Number of warehouses: %d\n",WAREHOUSES);
			init_shmem();
		}
		//na sétima linha são definidas as coordenadas, os produtos e as suas quantidades de cada armazém
		else if(lines == 7)
		{
			i = 0;

			do
			{
				par = 0;
				w = 0;
				word = strtok(line," ");
				ware_mem[i].name = (char *)malloc(strlen(word)*sizeof(char));
				strcpy(ware_mem[i].name,word);

				while(word != NULL)
				{
					//coordenada x
					if(tokens == 1)
					{
						ware_mem[i].coord_x = atoi(word);
					}
					//coordenada y
					else if(tokens == 2)
					{
						ware_mem[i].coord_y = atoi(word);
					}
					//produtos e suas quantidades
					else if(tokens == 4)
					{
						tokens--;
						par++;

						//produto
						if(par % 2 != 0)
						{
							ware_mem[i].product[w].name = (char *)malloc(strlen(word)*sizeof(char));
							strcpy(ware_mem[i].product[w].name,word);
						}
						//quantidade
						else
						{
							ware_mem[i].product[w].quantity = atoi(word);
							w++;
						}
					}

					ware_mem[i].n_products = w;
					word = strtok(NULL,"xy: ,");
					tokens++;
				}

				i++;
				tokens = 0;

			}while(fgets(line,1024,f)!=NULL && i < WAREHOUSES);
		}

		tokens = 0;
	}

	//ficheiro está vazio
	if(lines == 0)
	{
		printf("File is empty\n");
	}

	fclose(f);
}

//iniciliza as variáveis de estatística
void initialize_stats(Statistic *stat)
{
	stat->travels_made = 0;
	stat->total_encomendas = 0;
	stat->shipped_prods = 0;
	stat->loaded_prods = 0;
	stat->delivered_prods = 0;
	stat->derivered_requests = 0;
	stat->restocks = 0;
}

//adiciona stock aos armazéns
void update_stock(int n)
{
	Message message;
	int i;
	int load_var;
	int units;
	int product_id;

	while(1)
	{
		//recebe mensagem pela message queue
		msgrcv(msgq_id,&message,sizeof(message),n,0);
		load_var = 0;

		//se a mensagem tiver sido enviada pelo processo Simulador
		if(message.type == 1)
		{
			i = message.ware_id;
			units = message.quantity_prod;
			product_id = message.prod_id;

			//se a quantidade do produto não for igual ao limite máximo de stock
			if(ware_mem[i].product[product_id].quantity < MAX_STOCK)
			{
				//se faltarem mais de 20 unidades para a quantidade do produto atingir o limite máximo de stock
				if(MAX_STOCK - ware_mem[i].product[product_id].quantity > units)
				{
					//acrescenta 20 unidades ao stock do produto no armazém i
					ware_mem[i].product[product_id].quantity += units;

					//atualiza o ficheiro de log
					sprintf(logInfo, "Product %s stock updated in %d units, in Warehouse %d", ware_mem[i].product[product_id].name, units, ware_mem[i].id);
					insert_log(logInfo);

					printf("Warehouse %s received %d of %s\n",ware_mem[i].name,units,ware_mem[i].product[product_id].name);
				}
				//se faltarem menos de 20 unidades para a quantidade do produto atingir o limite máximo de stock
				else
				{
					//acrescenta as unidades restantes ao stock do produto no armazém i para atingir o limite máximo de stock
					int s = MAX_STOCK - ware_mem[i].product[product_id].quantity;
					ware_mem[i].product[product_id].quantity += MAX_STOCK - ware_mem[i].product[product_id].quantity;

					//atualiza o ficheiro de log
					sprintf(logInfo, "Product %s stock updated in %d units, in Warehouse %d", ware_mem[i].product[product_id].name, s, ware_mem[i].id);
					insert_log(logInfo);

					printf("Warehouse %s received %d of %s\n",ware_mem[i].name,units,ware_mem[i].product[product_id].name);
				}

				//atualiza dados estatísticos
				mem->restocks++;
				mem->loaded_prods+=units;

			}
			//se a quantidade do produto for igual ao limite máximo de stock, não acrescenta produtos
			else
			{
				//atualiza o ficheiro de log
				sprintf(logInfo, "Product %s stock reached maximum limit in Warehouse %d", ware_mem[i].product[product_id].name, ware_mem[i].id);
				insert_log(logInfo);

				printf("Stock of product %s in warehouse %s has reached the maximum.\n",ware_mem[i].product[product_id].name, ware_mem[i].name);
			}

			CHOOSE_WARE++;
		}
		//se a mensagem tiver sido enviada por um Drone
		else
		{
			printf("PROCESSING ORDER %d FOR DRONE %d\n",message.order_id - WAREHOUSES, message.drone_id );

			while(load_var < message.quantity_prod)
			{
				load_var++;
				sleep(1);
			}

			message.mtype = message.order_id;

			if(msgsnd(msgq_id,&message,sizeof(message),0) < 0)
			{
				perror("Error sending message.\n");
				exit(-1);
			}
		}
	}
}

//cria o named pipe
void init_pipe()
{
	if((mkfifo(FIFO, O_CREAT|O_EXCL|0600) < 0) && (errno != EEXIST))
	{
		perror("Error creating pipe");
		exit(-1);
	}

	if((fifo_id = open(FIFO, O_RDONLY|O_NONBLOCK)) < 0)
	{
		perror("Error opening pipe for reading");
		exit(-1);
	}
}

//inicialização
void init()
{
	sem = sem_get(1,1);

	//lê as configuações do ficheiro config.txt
	read_config("config.txt");

	drone_idp = (pthread_t*)malloc(DRONES * sizeof(pthread_t));
	drones = (Drone*)malloc(DRONES * sizeof(Drone));

	//cria message queue
	if((msgq_id = msgget(message_key,IPC_CREAT|0700)) < 0)
	{
		perror("Error creating message pipe");
		exit(-1);
	}

	//iniciliza as variáveis de estatística
	initialize_stats(mem);
}

//imprime as estatísticas no ecrã
void show_stats(int signum)
{

	printf("\nTotal travels: %d\n",mem->travels_made);
	printf("Total de Encomendas: %d\n",mem->total_encomendas);
	printf("Produtos enviados: %d\n",mem->shipped_prods);
	printf("Produtos Carregados: %d\n",mem->loaded_prods);
	printf("Produtos entregues: %d\n",mem->delivered_prods);
	printf("Total pedidos realizados: %d\n",mem->derivered_requests);
	printf("Total restocks: %d\n\n",mem->restocks);
	signal(SIGUSR1,show_stats);
}

//imprime informação sobre os armazéns
void print_warehouse_info(int n)
{
	int i = 0;
	Warehouse w = ware_mem[n];

	printf("Name: %s\nID: %d\nCoordinate x: %.2f\nCoordinate y: %.2f\n\n",w.name,w.id,w.coord_x,w.coord_y);
	printf("Has products: \n");

	while(i < w.n_products)
	{
		printf("Product: %s | Quantity: %d\n",w.product[i].name,w.product[i].quantity);
		i++;
	}
}

//cria os processos armazém
//cria o número de processos definido no ficheiro config.txt
void create_warehouses()
{
	int n = 0;
	signal(SIGUSR1,show_stats);

	for(n = 0; n < WAREHOUSES; n++)
	{
		if(fork() == 0)
		{
			ware_mem[n].id = getpid();
			ware_mem[n].idpr = n+1;

			//atualiza ficheiro de log
			sprintf(logInfo, "Process Warehouse %d with PID %d created", n, ware_mem[n].id);
			insert_log(logInfo);

			printf("Process Warehouse %d with PID %d  created\n", n, ware_mem[n].id);
			update_stock(n+1);
		}
	}
}

//gere o movimento dos drones
void drone_travel(Drone *drone, double x, double y, int flag)
{
	int result = -1;
	int current_state = drone->state;

	// move drones to warehouse

	//atualiza ficheiro de log
	sprintf(logInfo, "Drone %d a initiating movement towards destination", drone->idpr);
	insert_log(logInfo);

	printf("\n[%d]DRONE(%.2f,%.2f)",drone->idpr,drone->coord_x,drone->coord_y);
	printf(" to (%.2f,%.2f)\n",x,y);

	while(result != 0  && (drone->state == current_state))
	{
		if(flag)
		{
			usleep(100000);
		}

		//movimenta o drone
		result = move_towards(&(drone->coord_x),&(drone->coord_y),x,y);

		//se o drone se movimentar até ao armazém com sucesso
		if(result == 0)
		{
			printf("Drone %d moved correctly toward warehouse.\n",drone->idpr);

			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d moved sucessfully to destination", drone->idpr);
			insert_log(logInfo);

			//atualiza dados estatísticos
			mem->travels_made += 1;

			printf("Drone final position (%.2f , %.2f)\n\n",drone->coord_x,drone->coord_y);
		}
		//se o drone já se encontra no local de destino
		else if(result == -1)
		{
			printf("Drone %d was already in site.\n",drone->idpr);

			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d already in destination site", drone->idpr);
			insert_log(logInfo);
		}
		//se ocorrer um erro na trajetória do drone
		else if(result == -2)
		{
			printf("Trajectory turned an error.\n");

			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d didnt get to warehouse, trajectory error occureed", drone->idpr);
			insert_log(logInfo);
		}
		//se a caminho da base o drone receber um novo pedido e mudar de trajetória
		else if(drone->state != current_state )
		{
			printf("Trajectory to base was interrupted! Got new task\n");

			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d trajectory to base was interrupted! Got new task", drone->idpr);
			insert_log(logInfo);
		}
	}

	//atualiza ficheiro de log
	sprintf(logInfo, "Drone %d located at (%.2f , %.2f)", drone->idpr, drone->coord_x,drone->coord_y);
	insert_log(logInfo);
}

//atribui a base a um drone
void closest_base(Drone *drone)
{
	int i;
	double min_distance;
	double dist;

	for(i = 0; i < 4; i++)
	{
		if(i == 0)
		{
			drone->base_no = i;
			min_distance = distance(drone->coord_x,drone->coord_y,bases[drone->base_no][0],bases[drone->base_no][1]);
		}
		else
		{
			dist = distance(drone->coord_x,drone->coord_y,bases[i][0],bases[i][1]);

			if(dist < min_distance)
			{
				drone->base_no = i;
			}
		}
	}
}

//gere ações de um drone
void *do_some_work(void *id)
{
	int idt = *((int *)id);
	Message message;
	Drone *drone = (Drone *)malloc(sizeof(Drone));
	drone = &drones[idt-1];

	printf("DRONE %d WAITING\n",idt );

	while(shutdown != 1)
	{
		pthread_cond_wait(&cv,&mutex);
		//se o drone receber um pedido e mudar o seu estado para ocupado
		if(drone->state != 0)
		{
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex);
			sigprocmask (SIG_BLOCK, &block_ctrlc, NULL);

			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d moving to %s", drone->idpr, drone->warehouse_s->name);
			insert_log(logInfo);
			//drone dirige-se ao armazém
			drone_travel(drone,drone->warehouse_s->coord_x,drone->warehouse_s->coord_y,0);

			//carrega os produtos
			message.type = 2;
			message.mtype = drone->warehouse_s->idpr;
			message.drone_id = idt;
			message.order_id = WAREHOUSES + drone->order_id;
			message.quantity_prod = drone->quantity;

			printf("LOADING DRONE %d\n",drone->idpr);
			printf("DRONE %d REQUEST %d\n",drone->idpr, drone->quantity);

			//envia mensagem ao armazém
			if(msgsnd(msgq_id,&message,sizeof(message),0) < 0)
			{
				perror("Error sending message.\n");
				exit(-1);
			}

			//recebe mensagem de fim do carregamento
			msgrcv(msgq_id,&message,sizeof(message),message.order_id,0);
			printf("WAREHOUSE FINISHED LOADING PRODUCTS\n");

			//drone dirige-se ao ponto de entrega
			printf("to destiny \n");
			drone_travel(drone,drone->destiny_x,drone->destiny_y,0);
			mem->delivered_prods+=drone->quantity;
			mem->derivered_requests++;
			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d reached destination and delivered order %d", drone->idpr, drone->order_id);
			insert_log(logInfo);

			drone->state = 0;
			drone->order_id = 0;

			//drone dirige-se de volta à base mais próxima
			printf("Back to base %d\n",drone->base_no );
			closest_base(drone);
			drone_travel(drone,bases[drone->base_no][0],bases[drone->base_no][1],1);
			sigprocmask (SIG_UNBLOCK, &block_ctrlc, NULL);

			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d moving to closest base", drone->idpr);
			insert_log(logInfo);
		}
	}

	pthread_exit(NULL);

}

//cria threads drones
//cria o número de threads definido no ficheiro config.txt
void create_drones()
{
	int i = 0;
	int j = 0;
	int err;
	time_t t;
	srand((unsigned)time(&t));
	// pthread_mutex_lock(&mutex);

	while(i != DRONES)
	{
		sleep(1);

		drones[i].idpr = i+1;
		drones[i].coord_x = bases[j][0];
		drones[i].coord_y = bases[j][1];
		//a 0 o drone está livre
		drones[i].state = 0;
		drones[i].base_no = j;
		drones[i].loaded = 0;

		if((err = pthread_create(&drones[i].id, NULL, &do_some_work,&drones[i].idpr)) == 0)
		{
			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d created", drones[i].idpr);
			insert_log(logInfo);
		}
		else
		{
			printf("Thread failed to create.\n");
			exit(-1);
		}

		j = (j + 1)%4;
		i++;
	}

	sleep(2);
	printf("\n");
}

//abastece os armazéns
void wait_for_signal()
{
	Message message;
	message.type = 1;

	while(1)
	{
		sleep(FILL_FREQ);
		kill(getpid(),SIGUSR1);

		message.ware_id = CHOOSE_WARE % WAREHOUSES;
		message.mtype = (CHOOSE_WARE % WAREHOUSES) +1;
		message.quantity_prod = QUANTITY;
		message.prod_id = rand() % ware_mem[message.ware_id].n_products;

		if(msgsnd(msgq_id,&message,sizeof(message),0) < 0)
		{
			perror("Error sending message.\n");
			exit(-1);
		}

		CHOOSE_WARE++;
	}
}

//adiciona drones
void add_drones(int update)
{
	int i = DRONES;
	int j = 0;
	int err = 0;

	DRONES += update;
	drones = (Drone *) realloc(drones,update*sizeof(Drone));

	if(drones  == NULL)
	{
			perror("Memory was not reallocated correctly");
			exit(-1);
	}

	printf("UPDATE NUMBER OF DRONES FROM %d TO %d\n",i,DRONES);

	//cria o número de drones a adicionar
	while(i < DRONES)
	{
		sleep(1);

		drones[i].idpr = i+1;
		drones[i].coord_x = bases[j][0];
		drones[i].coord_y = bases[j][1];
		drones[i].state = 0;
		drones[i].base_no = j;
		drones[i].loaded = 0;

		//cria as threads drone
		if((err = pthread_create(&drones[i].id, NULL, &do_some_work,&drones[i].idpr)) == 0)
		{
			//atualiza ficheiro de log
			sprintf(logInfo, "Drone %d created", drones[i].idpr);
			insert_log(logInfo);
		}
		else
		{
			printf("thread failed to create.\n");
			exit(-1);
		}

		j = (j+1)%4;
		i++;
	}
}

//verifica se é um inteiro
int check_if_int(char dig[])
{
	int value = atoi(dig);
	int ret = 1;

	if(value == 0)
	{
		ret = 0;
	}
	else
	{
		ret = 1;
	}

	return ret;
}

//verifica se um produto existe
int check_prod_exists(char prod[])
{
	int i = 0;
	int flag = 0;

	while(i < PRODUCTS_NUMBER && flag == 0)
	{
		if(strcmp(prod, all_products[i]) == 0)
		{
			flag = 1;
		}

		i++;
	}

	return flag;
}

//reserva os produtos de uma encomenda de um drone no armazém a que este se dirige
void reserver(Warehouse *w_to, char *p_name, int res)
{
	int i;
	int j;

	for(i = 0; i < WAREHOUSES; i++)
	{
		if(strcmp(ware_mem[i].name,w_to->name) == 0)
		{
			for(j = 0; j < ware_mem[i].n_products; j++)
			{
				if(strcmp(ware_mem[i].product[j].name,p_name) == 0)
				{
					ware_mem[i].product[j].quantity -= res;
				}
			}
		}
	}
}

//seleciona o drone a efetuar uma encomenda e o armazém a que este se dirige
void select_poles()
{
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

	for(i = 0; i < WAREHOUSES; i++)
	{
		for(j = 0; j < ware_mem[i].n_products ; j++)
		{
			//se o armazém tiver o produto que a encomenda precisa
			if(strcmp(list_orders->next->order->prod_name,ware_mem[i].product[j].name) == 0)
			{
				//se o armazém tiver a quantidade do produto necessária
				if(ware_mem[i].product[j].quantity >= list_orders->next->order->quantity)
				{
					//classifica o armazém como capaz de satisfazer a encomenda
					capable[w] = ware_mem[i];
					w++;
				}
			}
		}
	}

	printf("There are %d warehouses with needed resources\n",w);

	//se houverem armazéns capazes de satisfazer a encomenda
	if(w > 0)
	{
		for(i = 0; i < DRONES; i++)
		{
			//se houver um drone livre
			if(drones[i].state == 0)
			{
				//verifica a distância entre o drone e o armazém, para saber qual o drone mais perto
				for(j = 0; j < w; j++)
				{
					if(d == 0)
					{
						min_distance = distance(drones[i].coord_x,drones[i].coord_y,capable[j].coord_x,capable[j].coord_y);
						min_distance += distance(capable[j].coord_x,capable[j].coord_y,x,y);

						receiver_ware = &capable[j];
						send_drone = &drones[i];

						printf("New minimum distance : %.2f\n",min_distance);

						d++;
					}
					else
					{
						dist = distance(drones[i].coord_x,drones[i].coord_y,capable[j].coord_x,capable[j].coord_y);
						dist += distance(capable[j].coord_x,capable[j].coord_y,x,y);

						if(dist < min_distance)
						{
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

		//reserva a quantidade de produtos necessária no armazém que pode satisfazer a encomenda
		reserver(receiver_ware,list_orders->next->order->prod_name,list_orders->next->order->quantity);

		//passa os atributos ao ponteiro send_drone para que, quando o drone estiver a satisfazer a encomenda,
		//já tenha todas as informações sobre esta ao seu dispor
		send_drone->warehouse_s = (Warehouse *)malloc(sizeof(Warehouse));
		send_drone->warehouse_s = receiver_ware;
		send_drone->order_id = list_orders->next->order->id;
		send_drone->destiny_x = list_orders->next->order->x;
		send_drone->destiny_y = list_orders->next->order->y;
		send_drone->state = 1;
		send_drone->quantity = list_orders->next->order->quantity;
		pthread_cond_broadcast(&cv);
		//retira a encomenda da lista de encomendas a realizar
		pop_node(list_orders->next,list_orders);

		printf("DEALER | Drone: %d to: %s , state %d order %d\n",send_drone->idpr, send_drone->warehouse_s->name, send_drone->state, send_drone->order_id);
		mem->total_encomendas++;
		mem->shipped_prods+=send_drone->quantity;
		//atualiza ficheiro de log
		sprintf(logInfo, "Order with ID %d dispatched by Drone %d to (%.2f,%.2f) loading at %s",send_drone->order_id,send_drone->idpr,send_drone->destiny_x,send_drone->destiny_y,send_drone->warehouse_s->name);
		insert_log(logInfo);
	}
	else
	{
		//atualiza ficheiro de log
		sprintf(logInfo, "Order with ID %d delayed by Central for lack of stock", list_orders->next->order->id);
		insert_log(logInfo);

		delay_order(list_orders->next->order->id, list_orders);
	}
}

//envia a ordem de encomenda através do pipe
void approve_order(char order[])
{
	Order *new_order = (Order *)malloc(sizeof(Order));
	char *word;
	int check = 0;
	int p_exists = 0;
	int i = 0;
	int lenght;
	int d_update;
	char key_words[20][256];

	if(order != NULL && strstr(order, "prod:") != NULL)
	{
		printf("[CENTRAL] Received new order (%s)\n",order);
		word = strtok(order," ,");

		while(word != NULL)
		{
			strcpy(key_words[i],word);
			if(check_if_int(word))
			{
				check++;
			}

			if(i == 1)
			{
				p_exists = check_prod_exists(word);
			}

			word = strtok(NULL," ,");
			i++;
		}

		lenght = i;

		//se o pedido feito através do pipe satisfizer as condições
		if(lenght == 6 && p_exists && check == 3)
		{
			//cria uma nova encomenda com os dados inseridos
			new_order->id = order_id++;
			new_order->prod_name = (char *)malloc(strlen(key_words[1])*sizeof(char));
			strcpy(new_order->prod_name,key_words[1]);
			new_order->quantity = atoi(key_words[2]);
			new_order->x = atoi(key_words[4]);
			new_order->y = atoi(key_words[5]);

			printf("REQUEST ACCEPTED.\n");

			//insere a encomenda na lista de encomendas
			insert_q(new_order,list_orders);

			//imprime a lista de encomendas
			print_q(list_orders);

			//atualiza ficheiro de log
			sprintf(logInfo, "Order containing %d units of %s with ID %d received",new_order->quantity, new_order->prod_name, new_order->id);
			insert_log(logInfo);
		}
		else
		{
			printf("REQUEST WAS REJECTED\n");

			//atualiza ficheiro de log
			sprintf(logInfo, "Order rejected, syntax incorrect");
			insert_log(logInfo);
		}
	}
	else if(strcmp(strtok(order," "),"Drones") == 0)
	{
		printf("[CENTRAL] Drones update \n");
		word = strtok(NULL," ");

		if(check_if_int(word))
		{
			//adiciona drones
			d_update = atoi(word);
			printf("Updating %d drones\n",d_update);
		}
		else
		{
			printf("REQUEST WAS REJECTED\n");
		}
	}

	if(list_orders->next != NULL)
	{
		select_poles();
	}
}

//lê o pedido realizado através do pipe
void read_request()
{
	char order[1024];
	char send_order[1024];
	printf("Listenning to pipe\n");

	while (shutdown != 1)
	{
    read(fifo_id, &order, 1024*sizeof(char));
		strcpy(send_order,order);
		approve_order(send_order);
		strcpy(order,"nothing\0");
		sleep(2);
  }
}

//processo gestor da simulação
int main()
{

	int i;
	//inicializa a queue
	list_orders = init_q();

	//chama a função de inicialização
	init();

	//atualiza ficheiro de log
	insert_log("\n\n------------ LOG STARTED AT");

	//fica á espera do sinal SIGINT
	signal(SIGINT,terminate);
	sigemptyset (&block_ctrlc);
	sigaddset (&block_ctrlc, SIGINT);
	printf("\n[%d] SIMULATION PROCESS\n",getpid());

	create_warehouses();

	//processo gestor da simulação cria o processo Central e os processos Armazém
	//o processo Central cria o pipe e as threads dos drones
	if((central = fork()) == 0)
	{
		sleep(WAREHOUSES);
		printf("\n[%d] CENTRAL\n",getpid());
		init_pipe();
		create_drones();
		read_request();
		for(i = 0; i<DRONES;i++)
			pthread_join(drones[i].id,NULL);
		exit(0);
	}
	else
	{
		wait_for_signal();
	}
}
