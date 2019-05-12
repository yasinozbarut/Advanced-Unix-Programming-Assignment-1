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
int TRANSCATION_ID = 0;
time_t start, end;
struct PRODUCT *productList;
struct TRANSACTION *transactionList;

pthread_mutex_t *mutex;
pthread_mutex_t sellerFuncMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t customerFuncMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t *sendToSeller;
pthread_mutex_t timerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t operationMutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;
int numberOfSellers = 0;
int numberOfCustomers = 0;
int numberOfProducts = 0;
int numberOfSimulationDays = 0;
int totalTransactions = 0, constNumberOfTransactions = 0;
int sellerLock = 0;
int elapsedTime = 0;

struct SELLER {
    int sellerID;
    char *type;
};

struct TRANSACTION {
    int transcationID;
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

void *timerFunc(){
    while(numberOfSimulationDays > 0){
    time(&start);  // start the timer 
    do {
        time(&end);
        elapsedTime = (int)difftime(end, start);    
    } while(elapsedTime < 10);  // run for ten seconds and decrease numberOfSimulationDays
    numberOfSimulationDays -= 1;
    totalTransactions = constNumberOfTransactions - 1;
    }
    pthread_exit(NULL);
}

void *customerFunc(void *paramCustomer) {
    //pthread_mutex_lock(&customerFuncMutex);
    struct CUSTOMER *customer = (struct CUSTOMER *) paramCustomer;
    //printf("Customer ID: %d\n", customer->customerID);
    sleep(1);

    while (numberOfSimulationDays > 0) {
        //printf("Started Customer ID: %d\n",customer->customerID);
        //printf("Seller Lock: %d\n",sellerLock);

        
        if (pthread_mutex_trylock(&mutex[sellerLock]) != EBUSY) {
            int productID = (rand() % (numberOfProducts) + 0);
            int operation = (rand() % (3) + 0);
            int operationAmount = (rand() % ((int)(productList[productID].totalProducts / 2) + 1) + 0);
            //printf("CID: %d - PID: %d - OP: %d - Amount: %d\n", customer->customerID, productID, operation, operationAmount);

            pthread_mutex_lock(&customerFuncMutex);
            if (totalTransactions > 0) {
                totalTransactions--;
            }
            pthread_mutex_unlock(&customerFuncMutex);
            TRANSCATION_ID++;
            transactionList[totalTransactions].transcationID = TRANSCATION_ID;
            transactionList[totalTransactions].customerID = customer->customerID;
            transactionList[totalTransactions].productID = productID;
            transactionList[totalTransactions].operation = operation;
            transactionList[totalTransactions].operationAmount = operationAmount;
            //printf("SIGNALLING: %d\n", sellerLock);
            pthread_cond_signal(&sendToSeller[sellerLock]);
            printf("Sim Day: %d -",numberOfSimulationDays);
            printf("TID: %d - CID: %d - PID: %d - OP: %d - OP Amount: %d - P Amount: %d\n", 
                transactionList[totalTransactions].transcationID,
                transactionList[totalTransactions].customerID, 
                transactionList[totalTransactions].productID, 
                transactionList[totalTransactions].operation, 
                transactionList[totalTransactions].operationAmount,
                productList[transactionList[totalTransactions].productID].totalProducts);

            //printf("Signalling: %d totalTransactions: %d\n\n", sellerLock, totalTransactions);

            pthread_mutex_unlock(&mutex[sellerLock]);
        }

        pthread_mutex_lock(&sellerFuncMutex);
        if (sellerLock == (numberOfSellers - 1)) {
            sellerLock = 0;
            sleep(1);
        }else{
            sellerLock++;
        }
        pthread_mutex_unlock(&sellerFuncMutex);
        //sleep(1);
    }
    //pthread_mutex_unlock(&customerFuncMutex);
    /** Incoming customer tries to lock seller if seller is EBUSY customer tries to lock next seller.
    * It goes forward for all sellers until it locks one.
    */
    pthread_exit(NULL);
    //struct transaction trans = {customer->customerID, 1, productID, buy, reserve, cancel};
}
//0 -> buy  1 -> reserve  2 -> cancel
void *sellerFunc(void *seller) {
    int sellerID = *((int *) seller) - 1;
    printf("\nSim Day: %d Seller: %d\n",numberOfSimulationDays,sellerID);
    while (numberOfSimulationDays > 0) {

        if (pthread_mutex_trylock(&mutex[sellerID]) != EBUSY && totalTransactions >= 0) {
            printf("Seller %d waiting for signal.\n", sellerID);
            pthread_cond_wait(&sendToSeller[sellerID], &mutex[sellerID]);
            printf("Signal came to Seller %d, totalTransactions: %d\n", sellerID, totalTransactions);
            pthread_mutex_lock(&operationMutex);
            int operation = transactionList[totalTransactions].operation;
            if(operation == 0 || operation == 1){
                productList[transactionList[totalTransactions].productID].totalProducts -= transactionList[totalTransactions].operationAmount;
            }else if(operation == 2){
                productList[transactionList[totalTransactions].productID].totalProducts += transactionList[totalTransactions].operationAmount;
            }
            pthread_mutex_unlock(&operationMutex);
            /*printf("PID: %d, Amount: %d\n", transactionList[totalTransactions].productID,
                   productList[transactionList[totalTransactions].productID].totalProducts);*/
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
                        //printf("numberOfSimulationDays: %d\n", numberOfSimulationDays);
                    } else if (!strcmp("products", temp[index])) {
                        numberOfProducts = atoi(token);
                        productList = (struct PRODUCT *) malloc(numberOfProducts * sizeof(struct PRODUCT));
                    }
                    //printf("%s ", temp[index]);
                    index++;
                }
            } // END OF 4 LINES
            if (i >= 4 && i < (numberOfProducts + 4)) {
                productList[i - 4].totalProducts = atoi(token);
                productList[i - 4].productID = (i - 4);
                char pr[] = "product";
                productList[i - 4].type = pr;

                //printf("Product ID: %d, Total Products: %d\n", products[i-4].productID, products[i-4].totalProducts);

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

    /*for (size_t i = 0; i < numberOfProducts; i++){
        printf("Product ID: %d, Total Products: %d\n", products[i].productID, products[i].totalProducts);
    }*/

    //numberOfSimulationDays = 1; // TODO: remove this line to switch next simulation day.
    //numberOfSimulationDays++;
    int seller_index;
    int customer_index;
    int rc = 0;

    constNumberOfTransactions = totalTransactions;

    pthread_t customerThreads[numberOfCustomers], timerThread;
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

    //timer thread created
    pthread_create(&timerThread, NULL, timerFunc, NULL);

    transactionList = (struct TRANSACTION *) malloc(totalTransactions * sizeof(struct TRANSACTION));
    //printf("\nSUMMARY: numberOfSellers: %d, numberOfCustomers: %d, numberOfSimulationDays: %d, numberOfProducts: %d\n\n",
    //       numberOfSellers, numberOfCustomers, numberOfSimulationDays, numberOfProducts);
    printf("Progress started.\n");
    // creation of seller threads
    for (seller_index = 0; seller_index < numberOfSellers; seller_index++) {
        //printf("Seller %d thread created.\n", seller_index);
        rc = pthread_create(&sellerThreads[seller_index], &pthreadAttr, sellerFunc, (void *) &seller_index);
    }

    //time(&start);  /* start the timer */

    //printf("Seller thread creation is completed.\n");
    // creation of customer threads
    for (customer_index = 0; customer_index < numberOfCustomers; customer_index++) {
        //printf("Customer %d thread created.\n", customer_index);
        rc = pthread_create(&customerThreads[customer_index], &pthreadAttr, customerFunc,
                            (void *) &customers[customer_index]);
    }
    //printf("Customer thread creation is completed.\n");
    
    
    /*
    time(&start);  // start the timer 
    do {
        time(&end);
        elapsedTime = (int)difftime(end, start);  
        if((elapsedTime % 10) == 0){
            numberOfSimulationDays -= 1;
        }     
    } while((elapsedTime % 10) == 0 && numberOfSimulationDays > 0);  // run for ten seconds and decrease numberOfSimulationDays
    */

    
    // thread joins
    for (customer_index = 0; customer_index < numberOfCustomers; customer_index++) {
        rc = pthread_join(customerThreads[customer_index], NULL);
    }

    
    for (seller_index = 0; seller_index < numberOfSellers; seller_index++) {
        
        printf("Seller Index: %d",seller_index);
        rc = pthread_join(sellerThreads[seller_index], NULL);

    }

    //scanf("%d");
    return 0;
} 
