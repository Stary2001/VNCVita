#include <stdint.h>
#include <stdlib.h>
#include "list.h"

struct list* list_create()
{
	struct list *l = malloc(sizeof(struct list));
        l->len = 0;
        l->head = l-> tail = l->curr = 0;
	return l;
}

void list_add(struct list *l, void *item)
{
	struct list_item *i = (struct list_item*) malloc(sizeof(struct list_item));
	i->next = NULL;
	i->ptr = item;

	if(l->tail == 0)
	{
		l->head = l->tail = l->curr = i;
	}
	else
	{
		l->tail->next = i;
		l->tail = i;
	}
}

void list_del(struct list *l, struct list_item *item)
{
	struct list_item *prev = NULL;
	
	struct list_item *i = l->head;
	while(i)
	{
		if(i == item)
		{
			prev->next = i->next;
			break;
		}
		prev = i;
		i = i -> next;
	}
}

void* list_get(struct list *l)
{
	if(l->curr == NULL)
		return NULL;
	return l->curr->ptr;
}

void list_reset(struct list *l)
{
	l->curr = l->head;
}
