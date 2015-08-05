struct list_item
{
	void *ptr;
	struct list_item *next;
};

struct list
{
	uint32_t len;
	struct list_item *head;
	struct list_item *tail;
        struct list_item *curr;
};

struct list* list_create();
void list_add(struct list *l, void* item);
void list_del(struct list *l, struct list_item *item);
void* list_get(struct list *l);
void list_reset(struct list *l);
