#ifndef __LIST_H
#define __LIST_H


typedef struct _list_t {
	struct _list_t *next;
	struct _list_t *prev;
	unsigned int key;
} list_t, elem_t;

#define list_for_each( pos, list )			for( pos = (list)->next; pos != (list); pos = pos->next )	
#define list_for_each_safe(pos, nxt, list)	for( pos = (list)->next, nxt = pos->next; pos != (list); pos = nxt, nxt = pos->next )

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void elem_init( elem_t* elem, unsigned int _key )
{
	elem->next = NULL;
	elem->prev = NULL;
	elem->key  = _key;
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void list_init( list_t* list )
{
	list->next = list;
	list->prev = list;
	list->key  = 0xffffffff;
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void __list_add( elem_t* elem, elem_t* prev, elem_t* next )
{
	next->prev = elem;	elem->next = next;
	elem->prev = prev;	prev->next = elem;
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void list_add_first( list_t* list, elem_t* elem )
{
	__list_add( elem, list, list->next );
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void list_add( list_t* list, elem_t* elem )
{
	__list_add( elem, list->prev, list );
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void __list_del( elem_t* prev, elem_t* next )
{
	next->prev = prev;
	prev->next = next;
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void list_del( elem_t* elem )
{
	__list_del(elem->prev, elem->next);
	elem->next = NULL;
	elem->prev = NULL;
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline void list_move( list_t* list, elem_t* elem )
{
	__list_del(elem->prev, elem->next);
	list_add( list, elem );
}

static
#ifdef INTEL_TM
[[TRANSACTION_ANNOTATION]]
#endif
inline int list_empty( list_t* list )
{
	return list->next == list;
}

#endif
