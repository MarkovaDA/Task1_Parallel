#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#define STRLEN 100
#define WORKERS 5     //количество рабочих потоков по умолчанию установлено равным 5

struct Stack
	{
		void *data;
		int size;
		int max_size;
		int type;
	};
	typedef struct Stack Stack;
	void stackInitialize(Stack *s, int _max_size, int _type);
	void stackPush(Stack *s, void *value);
	void stackPop(Stack *s, void *new_value);
	void stackDestroy(Stack *s);

//extern char *strtok_r(char *, const char *, char **);//функция для парсингa строки 
    Stack *String_stack; 
    pthread_mutex_t *String_stack_mutex;
    pthread_mutex_t *String_stack_cond_mutex;
    pthread_cond_t  *String_stack_condition;
    pthread_mutex_t *Read_done_mutex;
    int *Read_done;

    pthread_mutex_t *Int_stack_mutex;
    pthread_cond_t  *Int_stack_condition;
    pthread_mutex_t *Int_stack_cond_mutex;
    pthread_mutex_t *Read_done_mutex;

    pthread_mutex_t *Work_done_mutex;
    int *Work_done;

    Stack *Int_stack;

	
void * reader()
{
    FILE *fptr;
    fptr = fopen("input.txt", "r");
    char c[STRLEN];
    while(!feof(fptr))
    {
        if(fgets(c, STRLEN, fptr))
        {
            printf("reader have read line: %s", c);//ридер считывает строку
            pthread_mutex_lock(String_stack_mutex);
            stackPush(String_stack, c); //кладет её в строковый стек, при этом он обладает монопольным доступом к стеку
            pthread_mutex_unlock(String_stack_mutex);
	    pthread_mutex_lock(String_stack_cond_mutex);
            pthread_cond_broadcast(String_stack_condition); //уведомляем рабочие потоки, о том, что строка добавилась в стек
	    pthread_mutex_unlock(String_stack_cond_mutex);
        }
    }
	pthread_mutex_lock(Read_done_mutex);
	*Read_done = 1; //поднимаем флаг о том, что чтение закончилось
	pthread_mutex_unlock(Read_done_mutex);
    	fclose(fptr);
	printf("reader stopped\n");
}

void * worker(void * arg)
{
    int id = (int)pthread_self();
    printf("worker started id: %d\n", id);
    int condition = 1;
    while (condition)
    {
	pthread_mutex_lock(String_stack_mutex);
        if((*String_stack).size > 0)
        {
            char string[STRLEN];
            pthread_mutex_lock(String_stack_mutex);
            stackPop(String_stack, string);//извлекаем строку из стека
            pthread_mutex_unlock(String_stack_mutex);
	    pthread_mutex_unlock(String_stack_mutex);
            printf("worker %d got string: %s", id, string);
            const char separator[] = " \n";
            int sum = 0;
			int a = 0;
			char *z;
            char *res = strtok_r(string, separator, &z);
            while(res != NULL)
            {
				a = (int)strtol(res, NULL , 10);
				printf("worker %d res: %d\n", id, a);
                sum += a;
                res = strtok_r(NULL, separator, &z);
            }//парсим строку, считаем сумму цифр в ней
            printf("worker %d sum: %d\n", id, sum);
            pthread_mutex_lock(Int_stack_mutex);
            stackPush(Int_stack, &sum); //кладём сумму в стек
            pthread_mutex_unlock(Int_stack_mutex);
	    pthread_mutex_lock(Int_stack_cond_mutex);
            pthread_cond_signal(Int_stack_condition); //подаем сигнал
            pthread_mutex_unlock(Int_stack_cond_mutex);
            
        }
        else
        {
			pthread_mutex_unlock(String_stack_mutex);
			pthread_mutex_lock(Read_done_mutex);
			if(*Read_done)
			{
				pthread_mutex_unlock(Read_done_mutex);
				condition = 0;
				break;
			}
            pthread_mutex_unlock(Read_done_mutex);
            printf("worker %d waiting for strings\n", id);
			struct timespec timeout;
			clock_gettime(CLOCK_REALTIME, &timeout);
			timeout.tv_sec += 1;
            pthread_mutex_lock(String_stack_cond_mutex);
            pthread_cond_timedwait(String_stack_condition, String_stack_cond_mutex, &timeout);
            pthread_mutex_unlock(String_stack_cond_mutex);
        }
    }
	pthread_mutex_lock(Work_done_mutex);
	*Work_done += 1;
	pthread_mutex_unlock(Work_done_mutex);
	printf("worker stopped id:%d\n", id);
}

void * writer(void * arg)
{
    printf("writer started id: %d\n", (int)pthread_self());
    FILE *fptr;
    fptr = fopen("output.txt", "w");
    int result = 0;
	int condition = 1;
    while(condition)
    {
	pthread_mutex_lock(Int_stack_mutex);
        if(Int_stack->size > 0)
        {
			int i;
            pthread_mutex_lock(Int_stack_mutex);
			stackPop(Int_stack, &i);
            pthread_mutex_unlock(Int_stack_mutex);
			pthread_mutex_unlock(Int_stack_mutex);
			result += i;
            fptr = fopen("output.txt", "w");
            fprintf(fptr, "%d", result);
            fclose(fptr);
            printf("writer wrote result: %d\n", result);
        }
        else
        {

			pthread_mutex_unlock(Int_stack_mutex);
			pthread_mutex_lock(Work_done_mutex);
			printf("writer checks 'work_done' = %d\n", *Work_done);
			if(*Work_done == WORKERS)
			{
				pthread_mutex_unlock(Work_done_mutex);
				condition = 0;
				break;
			}
			pthread_mutex_unlock(Work_done_mutex);
            printf("writer waiting for sums\n");
			struct timespec timeout;
			clock_gettime(CLOCK_REALTIME, &timeout);
			timeout.tv_sec += 1;
            pthread_mutex_lock(Int_stack_cond_mutex);
            pthread_cond_timedwait(Int_stack_condition, Int_stack_cond_mutex, &timeout);
            pthread_mutex_unlock(Int_stack_cond_mutex);
        }
    }
	printf("writer stopped\n");
}

int main()
{
    Stack string_stack;
    stackInitialize(&string_stack, 100, 1);

    Stack int_stack;
    stackInitialize(&int_stack, 100, 0);

        pthread_mutex_t int_stack_mutex;
	pthread_mutex_t string_stack_mutex;
	
	pthread_mutexattr_t recursive_mutex_attr;
	pthread_mutexattr_init(&recursive_mutex_attr);
	pthread_mutexattr_settype(&recursive_mutex_attr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&int_stack_mutex, &recursive_mutex_attr);
	pthread_mutex_init(&string_stack_mutex, &recursive_mutex_attr);

    pthread_cond_t string_stack_condition = PTHREAD_COND_INITIALIZER;
    pthread_cond_t int_stack_condition = PTHREAD_COND_INITIALIZER;

    pthread_mutex_t string_stack_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t int_stack_cond_mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_t read_done_mutex = PTHREAD_MUTEX_INITIALIZER;
	int read_done = 0;	

	pthread_mutex_t work_done_mutex = PTHREAD_MUTEX_INITIALIZER;
	int work_done = 0;	

    String_stack = &string_stack;
    String_stack_mutex = &string_stack_mutex;
    String_stack_condition = &string_stack_condition;
    String_stack_cond_mutex = &string_stack_cond_mutex;
    Read_done = &read_done;
    Read_done_mutex = &read_done_mutex;

    pthread_t reader_t;
    pthread_create(&reader_t, NULL, reader, NULL);

    Int_stack = &int_stack;
    Int_stack_mutex = &int_stack_mutex;
    Int_stack_cond_mutex = &int_stack_cond_mutex;
    Int_stack_condition = &int_stack_condition;
	
	
    Work_done = &work_done;
    Work_done_mutex = &work_done_mutex;

    pthread_t worker_t[WORKERS];
    for(int i = 0; i < WORKERS; i++)
    {
        pthread_create(&worker_t[i], NULL, worker, NULL);
    }

    pthread_t writer_t;
    pthread_create(&writer_t, NULL, writer, NULL);


	if(!pthread_join(reader_t, NULL))
	{
		printf("Main: reader stopped!\n");
	}

	for(int i = 0; i < WORKERS; i++)
	{
		if(!pthread_join(worker_t[i], NULL))
		{
			printf("Main: worker[%d] stopped!\n", i+1);
		}
	}
	printf("Main: WORKERS STOPPED!\n");

    if(!pthread_join(writer_t, NULL))
	{
		printf("Main: writers stopped!\n");
	}
	
	stackDestroy(&string_stack);
	stackDestroy(&int_stack);

	return 0;
}
