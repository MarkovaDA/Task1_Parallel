#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct Stack
{
    void *data;
    int size;
    int max_size;
	int type;
};
typedef struct Stack Stack;


void stackInitialize(Stack *s, int _max_size, int _type)
{
	if(_type)
	{
    	s->data = (char**)malloc(sizeof(char*)*_max_size);
	}
	else
	{
		s->data = (int*)malloc(sizeof(int)*_max_size);
	}
	s->size = 0;
    s->max_size = _max_size;
	s->type = _type;
}


void stackPush(Stack *s, void *value)
{
	if(s->size < s->max_size)
	{
		if(s->type)
		{
			((char**)s->data)[s->size] = malloc(sizeof(char)*strlen((char *)value));
			strcpy(((char**)s->data)[s->size], (char*)value);
			
		}
		else
		{
			((int*)s->data)[s->size] = *(int*)value;
		}
		s->size++;
	}
	else
	{
		fprintf(stderr, "Error: Stack is full\n");
	}
}


void stackPop(Stack *s, void *new_value)
{
    if(s->size == 0)
    {
		fprintf(stderr, "Error: Stack is empty\n");
    }
    else
    {
		s->size--;
		if(s->type)
		{
			strcpy(new_value, ((char**)s->data)[s->size]);
			free(((char**)s->data)[s->size]);
		}
		else
		{
			*(int*)new_value = ((int*)s->data)[s->size];
		}
    }
}

void stackDestroy(Stack *s)
{
	free(s->data);
}
