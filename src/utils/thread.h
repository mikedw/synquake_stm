#ifndef THREAD_H_
#define THREAD_H_

#include "../general.h"

#define THREAD_POSIX

#ifdef THREAD_SDL

#include <SDL.h>
#include <SDL_thread.h>


typedef	SDL_mutex					mutex_t;

#define mutex_create				SDL_CreateMutex
#define mutex_destroy				SDL_DestroyMutex
#define mutex_lock					SDL_LockMutex
#define mutex_unlock				SDL_UnlockMutex


typedef	SDL_sem						semaphore_t;

#define semaphore_create			SDL_CreateSemaphore
#define semaphore_destroy			SDL_DestroySemaphore
#define semaphore_post				SDL_SemPost
#define semaphore_wait				SDL_SemWait

typedef SDL_cond                    cond_t;

#define cond_create                 SDL_CreateCond
#define cond_destroy                SDL_DestroyCond
#define cond_wait                   SDL_CondWait
#define cond_broadcast              SDL_CondBroadcast
#define cond_signal                 SDL_CondSignal

typedef	SDL_Thread					thread_t;

#define thread_create				SDL_CreateThread
#define thread_join(t)				SDL_WaitThread( t, NULL )

#endif



#ifdef THREAD_POSIX

#include <pthread.h>
#include <semaphore.h>


typedef	sem_t						semaphore_t;

#define semaphore_create()			generic_create2( sem_t, sem_init, 0, 0 )
#define semaphore_destroy(s)			generic_destroy( s, sem_destroy )
#define semaphore_post				sem_post
#define semaphore_wait				sem_wait

typedef	pthread_t					thread_t;

#define thread_create( run_f, a )	generic_create3( pthread_t, pthread_create, NULL, run_f, a )
#define thread_join(t)				pthread_join( *t, NULL )


#ifndef USE_POSIX_LOCKS

#define SLEEPING_STATE 1
#define NEUTRAL_STATE  0

#include "tatas.h"

typedef	tatas_lock_t				mutex_t;

#define mutex_create()				generic_create1 (tatas_lock_t, tatas_init, (void*)0)
#define mutex_destroy(m)			generic_free (m)
#define mutex_lock				tatas_lock
#define mutex_unlock				tatas_unlock

#define MAX_THREADS 16
typedef int            		    cond_t[MAX_THREADS]; 

#define cond_create()               generic_create (cond_t, fastcv_init);
#define cond_destroy(c)             generic_free (c)
#define cond_wait                   fastcv_wait
#define cond_broadcast              fastcv_broadcast
#define cond_signal                 fastcv_signal


int fastcv_init (cond_t *c);
int fastcv_wait (cond_t *c, tatas_lock_t *mutex);
int fastcv_signal (cond_t *c);
int fastcv_broadcast (cond_t *c);

#else

typedef	pthread_mutex_t				mutex_t;

#define mutex_create()				generic_create1( pthread_mutex_t, pthread_mutex_init, NULL )
#define mutex_destroy(m)			generic_destroy( m, pthread_mutex_destroy )
#define mutex_lock				pthread_mutex_lock
#define mutex_unlock				pthread_mutex_unlock

typedef pthread_cond_t              cond_t;

#define cond_create()               generic_create1( pthread_cond_t, pthread_cond_init, NULL )
#define cond_destroy(m)             generic_destroy(m, pthread_cond_destroy )
#define cond_wait                   pthread_cond_wait
#define cond_broadcast              pthread_cond_broadcast
#define cond_signal                 pthread_cond_signal

#endif // ! USE_POSIX_LOCKS

#endif

void thread_set_local_id( int tid );
void thread_set_affinity( int tid );

typedef struct
{
	int n, count;
	mutex_t* mutex;
	semaphore_t* turnstile;
	semaphore_t* turnstile2;
	cond_t* cond;
} barrier_t;

void barrier_init( barrier_t* b, int n );
void barrier_deinit( barrier_t* b );
void barrier_wait( barrier_t* b );

#define	barrier_create( n )			generic_create1( barrier_t, barrier_init, n )
#define barrier_destroy( b )		generic_destroy( b, barrier_deinit )

#endif /*THREAD_H_*/

