#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<errno.h>

struct product *products;
struct transaction *transactions;
pthread_mutex_t *mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t transactionCreationMutex = PTHREAD_MUTEX_INITIALIZER;
int numberOfSellers = 0, numberOfCustomers = 0, numberOfProducts = 0, numberOfSimulationDays = 0, totalTransactions = 0;


struct seller
{
	int sellerID;
	char *type;
};

struct transaction{
	int customerID;
	int simulationDay;
	int productID;
	int operationAmount;
	int operation;
};

struct product
{
	int totalProducts;
	int productID;
	char *type;
};

struct customer
{
	int customerID;
	int numberOfReservableProducts;
	int numberOfOperationsAllowed;
	char *type;
};

void *sellerFunc(void *seller){
	int sellerID = *((int*) seller);

}

void *customerFunc(void *customer1){
	struct customer *customer = (struct customer*) customer1;

	int productID = (rand() % (numberOfProducts +1) + 0);
	int operation = (rand() % (3) + 0);
	int operationAmount = (rand() % (products[productID].totalProducts/2 + 1) + 0);
	printf("%d %d %d\n", customer->customerID, customer->numberOfOperationsAllowed, customer->numberOfReservableProducts);
	//struct transaction trans = {customer->customerID, 1, productID, buy, reserve, cancel};
} 

int main(int argc, char const *argv[])
{
	FILE *file = fopen("input.txt", "r");
	char line[50];
	char *token = NULL, *temp[3];
	int index, i = 0, j = 0;
	
	struct seller *sellers;
	struct customer *customers;
	pthread_t *sellerThreads;
	if(file != NULL){
		while(fgets(line, sizeof line, file) != NULL){
			token = strtok(line, "\n\t\r ");
			index = 0;
			if(i < 4){
			while((temp[index] = strtok(NULL, "\n\t\r ")) != NULL){
				if(!strcmp("customers", temp[index])){
					numberOfCustomers = atoi(token);
					customers = (struct customer *)malloc(numberOfCustomers * sizeof(struct customer));
				}else if(!strcmp("sellers", temp[index])){
					numberOfSellers = atoi(token);
					sellers = (struct seller *)malloc(numberOfSellers * sizeof(struct seller));
					mutex = (pthread_mutex_t *)malloc(numberOfSellers * sizeof(pthread_mutex_t));
					sellerThreads = (pthread_t *)malloc(numberOfSellers * sizeof(pthread_t));
				}else if(!strcmp("days", temp[index])){
					numberOfSimulationDays = atoi(token);
				}else if(!strcmp("products", temp[index])){
					numberOfProducts = atoi(token);
					products = (struct product *)malloc(numberOfProducts * sizeof(struct product));
				}
				//printf("%s ", temp[index]);
				index++;
			}
			}
			if(i >= 4 && i < (numberOfProducts + 4)){
				products[i-4].totalProducts = atoi(token);
				products[i-4].productID = (i-4);
				char pr[] = "product";
				products[i-4].type = pr;
				//printf("ID: %d, totalProducts: %d", products[i-4].productID, products[i-4].totalProducts);
			}else if(i >= numberOfProducts + 4){
				int c = numberOfProducts + 4;
				customers[i-c].customerID = atoi(token);
				char cus[] = "customer";
				customers[i-c].type = cus;
				//printf("Index: %d, ID: %d ", i-c, customers[i-c].customerID);
				while((temp[index] = strtok(NULL, "\n\t\r ")) != NULL){					
					if(index == 0){
						customers[i-c].numberOfOperationsAllowed = atoi(temp[index]);
						totalTransactions += atoi(temp[index]);
						//printf("%d\t", totalTransactions);
						//printf("Reservable: %d ", customers[i-c].numberOfReservableProducts);
					}
					if(index == 1){
						customers[i-c].numberOfReservableProducts = atoi(temp[index]);
						//printf("Operations: %d", customers[i-c].numberOfOperationsAllowed);
					}
					index++;									
				}
			}
			//printf("\n");
			i++;
		}

	}
	//printf("numberOfCustomers: %d, numberOfSellers: %d, numberOfSimulationDays: %d, numberOfProducts: %d\n", numberOfCustomers, numberOfSellers, numberOfSimulationDays, numberOfProducts);
	
	int s, c, rc = 0;
	pthread_t customerThreads[numberOfCustomers];
	pthread_attr_t pta;
	pthread_attr_init(&pta);
  	pthread_attr_setdetachstate(&pta, PTHREAD_CREATE_JOINABLE);
  	mutex = (pthread_mutex_t *)malloc(numberOfSellers * sizeof(pthread_mutex_t));
  	transactions = (struct transaction *)malloc(totalTransactions * sizeof(struct transaction));
	for(c = 0; c < numberOfCustomers; c++){
		rc = pthread_create(&customerThreads[c], &pta, customerFunc,(void*)&customers[c]);
	}

	for(s = 0; s < numberOfSellers; s++){
		rc = pthread_create(&sellerThreads[s], &pta, sellerFunc,(void*) &s);
	}

	for(c = 0; c < numberOfCustomers; c++){
		rc = pthread_join(customerThreads[c], NULL);
		
	}

	for(s = 0; s < numberOfSellers; s++){
		rc = pthread_join(sellerThreads[s], NULL);
		
	}

	scanf("%d");
	return 0;
}