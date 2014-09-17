#include "thread.h"


#if defined( coredump ) || defined( cluster003 ) || defined( segfault )
#define		AVOID_SMT
#endif

__thread int __local_tid = -1;


void thread_set_local_id (int tid) {
   __local_tid = tid;
   assert (__local_tid != -1);
}

int get_contexts()
{
    int n_contexts = 0;
    char line[1024], search_str[] = "cpu MHz";

    FILE* fp = fopen("/proc/cpuinfo", "r");	assert( fp );

    while( fgets( line, 1024, fp ) != NULL )
        if (strstr(line, search_str) != NULL)	n_contexts++;

    fclose(fp);
    return n_contexts;
}

void thread_set_affinity( int tid )
{
	static int n_contexts = -1;
	if( n_contexts == -1 )
		n_contexts = get_contexts();

#ifdef AVOID_SMT
	int cpu_id = (tid*2)  % n_contexts;
	cpu_id += (tid % n_contexts) < (n_contexts/2) ? 0 : 1;
#else
	int cpu_id = tid % n_contexts;
#endif

	cpu_set_t mask;CPU_ZERO( &mask );
	CPU_SET( cpu_id, &mask );
	int rez = sched_setaffinity(0, sizeof(mask), &mask);
	assert( rez == 0 );
}

void barrier_init( barrier_t* b, int n )
{
	assert( b && n > 0 );
	b->count = 0;
	b->n = n;
	b->mutex = mutex_create();
	b->turnstile = semaphore_create();
	b->turnstile2 = semaphore_create();
	b->cond = cond_create();
}

void barrier_deinit( barrier_t* b )
{
	assert( b );
	mutex_destroy(b->mutex);
	semaphore_destroy(b->turnstile);
	semaphore_destroy(b->turnstile2);
	cond_destroy(b->cond);
}


void barrier_wait( barrier_t* b)
{
	mutex_lock(b->mutex);
	b->count++;
	if(b->count < b->n)
	{
		cond_wait(b->cond, b->mutex);
	}
	else
	{
		cond_broadcast(b->cond);
		b->count = 0;
	}
	mutex_unlock(b->mutex);
}

void barrier_wait2( barrier_t* b )
{
    int i;
    if( b == NULL ) return;

    mutex_lock( b->mutex );
    b->count++;
    if( b->count == b->n )
        for( i = 0; i < b->n; i++ )		semaphore_post( b->turnstile );
    mutex_unlock( b->mutex );
    semaphore_wait( b->turnstile );

    mutex_lock( b->mutex );
    b->count--;
    if( b->count == 0 )
        for( i = 0; i < b->n; i++ )		semaphore_post( b->turnstile2 );
    mutex_unlock( b->mutex );
    semaphore_wait( b->turnstile2 );
}


#ifndef USE_POSIX_LOCKS

int fastcv_init (cond_t *c) {

   int i = 0;
   for (i = 0; i < MAX_THREADS; i ++) {
      (*c)[i] = NEUTRAL_STATE; 
   }
   return 0;
}


int fastcv_wait (cond_t *c, tatas_lock_t *mutex) {
   
   int b = 64;
   assert (__local_tid != -1);
   
   (*c) [__local_tid] = SLEEPING_STATE;
   
   while ((*c)[__local_tid] == SLEEPING_STATE) {
      tatas_unlock (mutex);
      backoff (b);
      tatas_lock (mutex);
   }
   
   return 0;
}


int fastcv_signal (cond_t *c) {

   int i = 0;
   assert (__local_tid != -1);

   for (i = 0; i < MAX_THREADS; i ++) {
      if ((i != __local_tid) && ((*c) [i] == SLEEPING_STATE)) {
         (*c) [i] = NEUTRAL_STATE;
         break;
      }
   }
   
   return 0;
}


int fastcv_broadcast (cond_t *c) {

   int i = 0;
   assert (__local_tid != -1);

   for (i = 0; i < MAX_THREADS; i ++) {
      if ((*c) [i] == SLEEPING_STATE) {
         (*c) [i] = NEUTRAL_STATE;
      }
   }

   return 0;
}

#endif // USE_POSIX_LOCKS
