#ifndef _LIST_H
#define _LIST_H

#define offset_of(type, member) ((size_t)&(((type*)0)->member))
#define container_of(ptr, type, member) ((type*)(((char*)(ptr))-offset_of(type, member)))

struct list
{
	struct list *next, *prev;
};

static inline void list_init(struct list *head)
{
	head->next = head->prev = head;
}

static void __list_add(struct list *prev, struct list *next, struct list *new)
{
	new->prev = prev;
	new->next = next;
	prev->next = new;
	next->prev = new;
}

static inline void list_add(struct list *head, struct list *new)
{
	__list_add(head, head->next, new);
}

static inline void list_add_tail(struct list *head, struct list *new)
{
	__list_add(head->prev, head, new);
}

static void __list_del(struct list *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

static inline void list_del(struct list *node)
{
	__list_del(node);
}

static inline char list_empty(struct list *head)
{
	return head->next == head;
}

#define list_foreach(head_p, trav_p, type, member)\
	for (struct list *__i = (head_p)->next;\
		__i != (head_p) && ({ trav_p = container_of(__i, type, member) ; 1; });\
		__i = __i->next)

#endif /* _LIST_H */

