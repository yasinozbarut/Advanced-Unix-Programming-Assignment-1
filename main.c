#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<errno.h>
#include<time.h>
#include<unistd.h>

// VISIT: https://deadlockempire.github.io

// TODO: CHECK: The pthread_cond_broadcast() function shall unblock all threads currently
// blocked on the specified condition variable cond.
// To skip next simulation day pthread_cond_broadcast() may be used.

time_t start, end;
double elapsed = 0;
struct PRODUCT *products;
struct TRANSACTION *transactions;

pthread_mutex_t *mutex;
pthread_mutex_t sellerFuncMutex = PTHREAD_MUTEX_INITIALIZER, customerFuncMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t *sendToSeller;
//pthread_mutex_t transactionCreationMutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0, numberOfSellers = 0, numberOfCustomers = 0, numberOfProducts = 0, numberOfSimulationDays = 0, totalTransactions = 0;
int sellerLock = 0;

struct SELLER {
    int sellerID;
    char *type;
};

struct TRANSACTION {
    int customerID;
    int simulationDay;
    int productID;
    int operationAmount;
    int operation;
};

struct PRODUCT {
    int totalProducts;
    int productID;
    char *type;
};

struct CUSTOMER {
    int customerID;
    int numberOfReservableProducts;
    int numberOfOperationsAllowed;
    char *type;
};

/** void *sellerFunc: void *seller
 * Bu fonksiyon customerFunc'tan gelen sendToSeller sinyalini sellerLock değişmeden
 * yakalayıp işleme sokuyor. Ayrıca productların sayısını düşürüyor.
 */

void *customerFunc(void *customer1) {
	//pthread_mutex_lock(&customerFuncMutex);
    struct CUSTOMER *customer = (struct CUSTOMER *) customer1;
    
    sleep(100);
    while(numberOfSimulationDays > 0 && totalTransactions > 0){
        if(pthread_mutex_trylock(&mutex[sellerLock]) != EBUSY ){
            int productID = (rand() % (numberOfProducts + 1) + 0);
            int operation = (rand() % (3) + 0);
            int operationAmount = (rand() % (products[productID].totalProducts / 2 + 1) + 0);
            printf("CUSTOMER\nproductID: %d operation: %d operationAmount: %d\n", productID, operation, operationAmount);
            pthread_mutex_lock(&customerFuncMutex);
            totalTransactions--;
            pthread_mutex_unlock(&customerFuncMutex);
            transactions[totalTransactions].customerID = customer->customerID;
            transactions[totalTransactions].productID = productID;
            transactions[totalTransactions].operation = operation;
            transactions[totalTransactions].operationAmount = operationAmount;
            pthread_cond_signal(&sendToSeller[sellerLock]);
            printf("Signalling: %d totalTransactions: %d\n\n", sellerLock, totalTransactions);
            pthread_mutex_unlock(&mutex[sellerLock++]);
        }
        pthread_mutex_lock(&sellerFuncMutex);
        if(sellerLock == (numberOfSellers-1)){
            sellerLock = 0;
        }
        pthread_mutex_unlock(&sellerFuncMutex);
    }
    //pthread_mutex_unlock(&customerFuncMutex);
    /** Incoming customer tries to lock seller if seller is EBUSY customer tries to lock next seller.
    * It goes forward for all sellers until it locks one.
    */
    pthread_exit(NULL);
    //struct transaction trans = {customer->customerID, 1, productID, buy, reserve, cancel};
}

void *sellerFunc(void *seller) {
    int sellerID = *((int*) seller);
    //printf("sellerID: %d\n", sellerID);
	while(numberOfSimulationDays > 0){
		if(pthread_mutex_trylock(&mutex[sellerID]) != EBUSY){
		printf("SELLER\nWaiting for signal: %d\n\n",sellerID);
		pthread_cond_wait(&sendToSeller[sellerID], &mutex[sellerID]);
		printf("Signal came: %d, totalTransactions: %d\n", sellerID, totalTransactions);
		products[transactions[totalTransactions].productID].totalProducts -= transactions[totalTransactions].operationAmount;
		printf("productID: %d, totalProducts: %d\n\n", transactions[totalTransactions].productID, products[transactions[totalTransactions].productID].totalProducts);
		pthread_mutex_unlock(&mutex[sellerID]);
        }
	}
    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    FILE *file = fopen("input.txt", "r"); // input file
    char line[50];
    char *token = NULL;
    char *temp[3];
    int index, i = 0, j = 0;

    struct SELLER *sellers;
    struct CUSTOMER *customers;
    pthread_t *sellerThreads;

	// FILE INPUT READ 
    if (file != NULL) {
        while (fgets(line, sizeof line, file) != NULL) {
            token = strtok(line, "\n\t\r ");
            index = 0;
            if (i < 4) { // FIRST 4 LINES
                while ((temp[index] = strtok(NULL, "\n\t\r ")) != NULL) {
                    if (!strcmp("customers", temp[index])) {
                        numberOfCustomers = atoi(token);
                        customers = (struct CUSTOMER *) malloc(numberOfCustomers * sizeof(struct CUSTOMER));
                    } else if (!strcmp("sellers", temp[index])) {
                        numberOfSellers = atoi(token);
                        sellers = (struct SELLER *) malloc(numberOfSellers * sizeof(struct SELLER));
                        mutex = (pthread_mutex_t *) malloc(numberOfSellers * sizeof(pthread_mutex_t));
                        sellerThreads = (pthread_t *) malloc(numberOfSellers * sizeof(pthread_t));
                    } else if (!strcmp("days", temp[index])) {
                        numberOfSimulationDays = atoi(token);
                    } else if (!strcmp("products", temp[index])) {
                        numberOfProducts = atoi(token);
                        products = (struct PRODUCT *) malloc(numberOfProducts * sizeof(struct PRODUCT));
                    }
                    //printf("%s ", temp[index]);
                    index++;
                }
            } // END OF 4 LINES
            if (i >= 4 && i < (numberOfProducts + 4)) {
                products[i - 4].totalProducts = atoi(token);
                products[i - 4].productID = (i - 4);
                char pr[] = "product";
                products[i - 4].type = pr;
                printf("ID: %d, totalProducts: %d\n", products[i-4].productID, products[i-4].totalProducts);
            } else if (i >= numberOfProducts + 4) {
                int c = numberOfProducts + 4;
                customers[i - c].customerID = atoi(token);
                char cus[] = "customer";
                customers[i - c].type = cus;
                //printf("Index: %d, ID: %d ", i-c, customers[i-c].customerID);
                while ((temp[index] = strtok(NULL, "\n\t\r ")) != NULL) {
                    if (index == 0) {
                        customers[i - c].numberOfOperationsAllowed = atoi(temp[index]);
                        totalTransactions += atoi(temp[index]);
                        //printf("%d\t", totalTransactions);
                        //printf("Reservable: %d ", customers[i-c].numberOfReservableProducts);
                    }
                    if (index == 1) {
                        customers[i - c].numberOfReservableProducts = atoi(temp[index]);
                        //printf("Operations: %d", customers[i-c].numberOfOperationsAllowed);
                    }
                    index++;
                }
            }
            //printf("\n");
            i++;
        }

    } // END OF FILE INPUT READ
    //printf("numberOfCustomers: %d, numberOfSellers: %d, numberOfSimulationDays: %d, numberOfProducts: %d\n", numberOfCustomers, numberOfSellers, numberOfSimulationDays, numberOfProducts);

    int seller_index;
	int customer_index;
	int rc = 0;
	

    pthread_t customerThreads[numberOfCustomers];
    pthread_attr_t pthreadAttr;
    pthread_attr_init(&pthreadAttr);
    pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_JOINABLE);

    mutex = (pthread_mutex_t *) malloc(numberOfSellers * sizeof(pthread_mutex_t));

    sendToSeller = (pthread_cond_t *) malloc(numberOfSellers * sizeof(pthread_cond_t));

    int mutexInitializer;
    for (mutexInitializer = 0; mutexInitializer < numberOfSellers; mutexInitializer++) {
        /* Initialize mutex and condition variable objects */
        pthread_mutex_init(&mutex[mutexInitializer], NULL);
        pthread_cond_init(&sendToSeller[mutexInitializer], NULL);
    }

    transactions = (struct TRANSACTION *) malloc(totalTransactions * sizeof(struct TRANSACTION));
    printf("numberOfSellers: %d, numberOfCustomers: %d, numberOfSimulationDays: %d, numberOfProducts: %d\n", numberOfSellers, numberOfCustomers, numberOfSimulationDays, numberOfProducts);
	// creation of seller threads
    for (seller_index = 0; seller_index < numberOfSellers; seller_index++) {
        //printf("seller_index: %d\n", seller_index);
        rc = pthread_create(&sellerThreads[seller_index], &pthreadAttr, sellerFunc, (void *) &seller_index);
    }

    // creation of customer threads
    for (customer_index = 0; customer_index < numberOfCustomers; customer_index++) {
        rc = pthread_create(&customerThreads[customer_index], &pthreadAttr, customerFunc,
                            (void *) &customers[customer_index]);
    }

	// thread joins

    for (customer_index = 0; customer_index < numberOfCustomers; customer_index++) {
        rc = pthread_join(customerThreads[customer_index], NULL);

    }

    for (seller_index = 0; seller_index < numberOfSellers; seller_index++) {
        rc = pthread_join(sellerThreads[seller_index], NULL);

    }

    //scanf("%d");
    return 0;
} 