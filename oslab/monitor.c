#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;
void* provider(void* arg) {
	while(1) {
		sleep(1);
		pthread_mutex_lock(&mutex);
        	while(ready){
                pthread_cond_wait(&condition, &mutex);
        	}
        ready=1;
        printf("Postavchik|sob otpravleno\n");
        pthread_cond_signal(&condition); 
        pthread_mutex_unlock(&mutex);
	}
        return NULL;
}
void* potreb(void* arg) {
	while(1) {
		pthread_mutex_lock(&mutex);
		while (!ready) {
			pthread_cond_wait(&condition, &mutex);
		}
	printf("potrebitel sob polucheno\n");
	ready = 0;
	pthread_cond_signal(&condition);
	pthread_mutex_unlock(&mutex);
	}
	return NULL;
}
int main()
{ 
        pthread_t t1,t2;
        pthread_create(&t1,NULL, provider, NULL);
        pthread_create(&t2,NULL, potreb, NULL);
        pthread_join(t1, NULL);                           
        pthread_join(t2, NULL); 
}

