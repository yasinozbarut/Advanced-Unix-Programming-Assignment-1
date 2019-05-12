#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<errno.h>
#include<time.h>
#include<unistd.h>


pthread_mutex_t *mutex;
pthread_mutex_t sellerFuncMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t customerFuncMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t timerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t operationMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t *sendToSeller; // signal that seller waits for

int counter = 0;
int numberOfSellers = 0;
int numberOfCustomers = 0;
int numberOfProducts = 0;
int numberOfSimulationDays = 0;
int totalTransactions = 0;
int constNumberOfTransactions = 0;
int sellerLock = 0;
int elapsedTime = 0;
int TRANSCATION_ID = 0;

time_t start;
time_t end;

struct PRODUCT *productList;
struct TRANSACTION *transactionList;
struct SELLER *sellers;
struct CUSTOMER *customers;

struct SELLER {
    int sellerID;
    int sellerTransactionCount;
    char *type;
};

struct TRANSACTION {
    int transcationID;
    int customerID;
    int sellerID;
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
    int customerTransactionCount;
    char *type;
};

void logger(){
    FILE *output ;
            output = fopen("output.txt", "w+");
            if (!output) {
                fprintf(output, "Error opening output.txt for writing.\n");
                return;
            }
            fprintf(output,"Sim Day: %d - TID: %d - CID: %d - PID: %d - OP: %d - OP Amount: %d - P Amount: %d\n",
                   numberOfSimulationDays,
                   transactionList[totalTransactions].transcationID,
                   transactionList[totalTransactions].customerID,
                   transactionList[totalTransactions].productID,
                   transactionList[totalTransactions].operation,
                   transactionList[totalTransactions].operationAmount,
                   productList[transactionList[totalTransactions].productID].totalProducts);
            fclose(output);
}

void *timerFunc() {
    while (numberOfSimulationDays > 0) {
        time(&start);  // start the timer
        printf("Day %d\n",numberOfSimulationDays);   
        do {
            time(&end);
            elapsedTime = (int) difftime(end, start);
        } while (elapsedTime < 10);  // run for ten seconds and decrease numberOfSimulationDays
        numberOfSimulationDays -= 1; // reduce simulation day counter
        totalTransactions = constNumberOfTransactions - 1;
    }

    pthread_exit(NULL);
}

void *customerFunc(void *paramCustomer) {
    
    struct CUSTOMER *customer = (struct CUSTOMER *) paramCustomer;

    while (numberOfSimulationDays > 0) {

        // if customer finds a seller who is unlocked, it lock it for itself
        if (pthread_mutex_trylock(&mutex[sellerLock]) != EBUSY) { 
            int productID = (rand() % (numberOfProducts) + 0);
            int operation = (rand() % (3) + 0);
            int operationAmount = (rand() % ((int) (productList[productID].totalProducts / 2) + 1) + 0);
            
            pthread_mutex_lock(&customerFuncMutex);
            if (totalTransactions > 0) {
                totalTransactions--;
            }
            pthread_mutex_unlock(&customerFuncMutex);

            TRANSCATION_ID++; // next transaction ID
            // create transaction in list
            transactionList[totalTransactions].transcationID = TRANSCATION_ID;
            transactionList[totalTransactions].customerID = customer->customerID;
            transactionList[totalTransactions].productID = productID;
            transactionList[totalTransactions].operation = operation;
            transactionList[totalTransactions].operationAmount = operationAmount;

            pthread_cond_signal(&sendToSeller[sellerLock]); // send signal to seller 

            printf("Sim Day: %d - TID: %d - CID: %d - PID: %d - OP: %d - OP Amount: %d - P Amount: %d\n",
                   numberOfSimulationDays,
                   transactionList[totalTransactions].transcationID,
                   transactionList[totalTransactions].customerID,
                   transactionList[totalTransactions].productID,
                   transactionList[totalTransactions].operation,
                   transactionList[totalTransactions].operationAmount,
                   productList[transactionList[totalTransactions].productID].totalProducts);
            pthread_mutex_unlock(&mutex[sellerLock]); // unlock seller mutex lock
        }
        pthread_mutex_lock(&sellerFuncMutex);
        if (sellerLock == (numberOfSellers - 1)) {
            sellerLock = 0;
            sleep(1);
        } else {
            sellerLock++;
        }
        pthread_mutex_unlock(&sellerFuncMutex);

    }
    pthread_exit(NULL);
}

//OPERATION CODES: 0 -> buy,  1 -> reserve,  2 -> cancel

void *sellerFunc(void *seller) {
    int sellerID = *((int *) seller) - 1;
    while (numberOfSimulationDays > 0) {
        if (pthread_mutex_trylock(&mutex[sellerID]) != EBUSY && totalTransactions >= 0) { // locks seller
            pthread_cond_wait(&sendToSeller[sellerID], &mutex[sellerID]);// gets signal to seller
            logger();
            transactionList[totalTransactions].sellerID = sellerID;
            pthread_mutex_lock(&operationMutex); // locks for product stocks
            int operation = transactionList[totalTransactions].operation;
            if ((operation == 0 || operation == 1) 
                && productList[transactionList[totalTransactions].productID].totalProducts >= 
                transactionList[totalTransactions].operationAmount) { // buy and reserve decreases stock.
                productList[transactionList[totalTransactions].productID].totalProducts -= transactionList[totalTransactions].operationAmount;
            } else if (operation == 2) { // cancel increases product stock
                productList[transactionList[totalTransactions].productID].totalProducts += transactionList[totalTransactions].operationAmount;
            }
            pthread_mutex_unlock(&operationMutex); // done with stock calculation and reveal lock
            
            pthread_mutex_unlock(&mutex[sellerID]); // done with seller unlocks seller lock
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    
    FILE *file = fopen("input.txt", "r"); // input file
    
    char line[50];
    char *token = NULL;
    char *temp[3];

    int index=0;
    int i = 0;
    int j = 0;

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
                        productList = (struct PRODUCT *) malloc(numberOfProducts * sizeof(struct PRODUCT));
                    }
                    index++;
                }
            } // END OF 4 LINES
            if (i >= 4 && i < (numberOfProducts + 4)) { 
                productList[i - 4].totalProducts = atoi(token);
                productList[i - 4].productID = (i - 4);
                char pr[] = "product";
                productList[i - 4].type = pr;
            } else if (i >= numberOfProducts + 4) {
                int c = numberOfProducts + 4;
                customers[i - c].customerID = atoi(token);
                char cusString[] = "customer";
                customers[i - c].type = cusString;
                while ((temp[index] = strtok(NULL, "\n\t\r ")) != NULL) {
                    if (index == 0) {
                        customers[i - c].numberOfOperationsAllowed = atoi(temp[index]);
                        totalTransactions += atoi(temp[index]);
                    }
                    if (index == 1) {
                        customers[i - c].numberOfReservableProducts = atoi(temp[index]);
                    }
                    index++;
                }
            }
            i++;
        }
    } // END OF FILE INPUT READ
    
    int seller_index;
    int customer_index;
    int rc = 0;

    constNumberOfTransactions = totalTransactions;

    pthread_t customerThreads[numberOfCustomers]; // customer thread number limitation
    pthread_t timerThread;

    // thread attribute
    pthread_attr_t pthreadAttr;
    pthread_attr_init(&pthreadAttr);
    pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_JOINABLE); // change threads attribute to joinable

    mutex = (pthread_mutex_t *) malloc(numberOfSellers * sizeof(pthread_mutex_t));

    sendToSeller = (pthread_cond_t *) malloc(numberOfSellers * sizeof(pthread_cond_t));

    int mutexInitializerIndex;
    for (mutexInitializerIndex = 0; mutexInitializerIndex < numberOfSellers; mutexInitializerIndex++) {
        /* Initialize mutex and condition variable objects */
        pthread_mutex_init(&mutex[mutexInitializerIndex], NULL);
        pthread_cond_init(&sendToSeller[mutexInitializerIndex], NULL);
    }

    //timer thread created
    pthread_create(&timerThread, NULL, timerFunc, NULL);

    transactionList = (struct TRANSACTION *) malloc(totalTransactions * sizeof(struct TRANSACTION));
    /*printf("\nSUMMARY: numberOfSellers: %d, numberOfCustomers: %d, numberOfSimulationDays: %d, numberOfProducts: %d\n\n",
           numberOfSellers, numberOfCustomers, numberOfSimulationDays, numberOfProducts);
    */

    printf("Progress started.\n");

    // creation of seller threads
    for (seller_index = 0; seller_index < numberOfSellers; seller_index++) {
        //printf("Seller %d thread created.\n", seller_index);
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
    
    return 0;
}
