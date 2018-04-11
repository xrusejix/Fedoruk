#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
 


#define _POSIX_BARRIERS 1

#define F(x,t)  25.0
#define DELTA_T 1.0
#define DELTA_X 1.0
#define A       1.0
 

 
struct timeval tv1;
struct timezone tz;
 
int timeOfWork()
{ 
  struct timeval tv2,dtv;
  gettimeofday(&tv2, &tz);
  dtv.tv_sec  = tv2.tv_sec - tv1.tv_sec;
  dtv.tv_usec = tv2.tv_usec - tv1.tv_usec;
  if(dtv.tv_usec < 0) 
    {
      dtv.tv_sec--;
      dtv.tv_usec += 1000000;
    }
  return dtv.tv_sec * 1000 + dtv.tv_usec / 1000;
}
 
typedef struct 
{
  pthread_t tid;
  int first;
  int last;
} ThreadRecord;
 
int CurrentTime = 0;
int **Z;
int node,nc;
int done = 0;
int precision = 1;
 
ThreadRecord *threads; 
pthread_barrier_t barr1, barr2;
 
FILE *out,*fp;
 
void writeIntoFile(int t)
{
  if( nc % precision == 0 )
  {
    int j;
    for(j = 0;j < node; j++)
      fprintf(out,"%d\t%d\n",j,Z[t][j]);
    fprintf(out, "\n\n");
  }
}
 
void* mysolver (void *arg_p) 
{
  ThreadRecord* thr;
  thr = (ThreadRecord*) arg_p;
  int i, f, cur, prev; 
  while ( !done ) 
  {
    pthread_barrier_wait(&barr1);
 
    cur  = CurrentTime % 2;
    prev = (CurrentTime + 1) % 2;
 
    for (i = thr->first; i <= thr->last; i++)
    {
      if ( i == (node-2) / 2 && CurrentTime < nc/20 )
        f = F(i,CurrentTime);
      else
        f = 0;
 
      Z[cur][i] = DELTA_T * DELTA_T * ( A * A * (Z[prev][i+1]-2*Z[prev][i]+Z[prev][i-1]) / (DELTA_X * DELTA_X )  + f + 2 * Z[prev][i]-Z[cur][i]);  
    }    
 
    pthread_barrier_wait(&barr2);
  }
 
    pthread_exit(0); 
}
 
void calculateFisrtTime(int nc)
{
  int i;
 
  for(i = 1; i < node-1; i++)
    Z[0][i] = Z[1][i] = 0;
 
  for(i = 0; i < 2; i++)
    writeIntoFile(i);
}
 
int main (int argc, char* argv[]) 
{
  if (argc != 4) 
   {
    fprintf(stderr,"# потоки интервал узлы\n");
    exit (1);
   }
 
  int nt = atoi(argv[1]);   // количество потоков
  nc = atoi(argv[2]);       // временной интервал  
  node = atoi(argv[3]);     // количество узлов
 
  if ( (node-2) % nt != 0 )
  {
    printf("enter kratnoe: %d\n",(node-2));
    scanf("%d",&nt);
  }
 
  if ( nt >= node-2 ) 
    nt = node-2;
 
  out = fopen("out.txt","w");
  fp = fopen("vgraph0.dat","w");              //открытие файла для графического представления в gnuplot
  fprintf(fp,"set xrange[0:%d]\nset yrange[0:70]\n", node-1);
  fprintf(fp, "do for [i=0:%d]{\n",nc/precision-1 );
  fprintf(fp,"plot 'out.txt' index i using 1:2 smooth bezier title 'powered by Seva'\npause 0.5}\npause -1\n");
 
  int i,k;
  Z = (int **) malloc (2 * sizeof(int*)); 
  for(i = 0; i < 2;i++)
    Z[i] = (int *) malloc (node * sizeof(int));
 
  calculateFisrtTime(nc);
 
  pthread_attr_t pattr;
  pthread_attr_init (&pattr);
  pthread_attr_setscope (&pattr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate (&pattr,PTHREAD_CREATE_JOINABLE);
 
  threads = (ThreadRecord *) calloc (nt, sizeof(ThreadRecord));
 
  pthread_barrier_init(&barr1, NULL, nt+1);
  pthread_barrier_init(&barr2, NULL, nt+1);
 
 
  int j = (node - 2) / nt; 
 
  for (i = 0,k = 1; i < nt; i++,k+=j)  
  {
    threads[i].first = k;         
    threads[i].last  = k+j-1;
    if ( pthread_create (&(threads[i].tid), &pattr, mysolver, (void *) &(threads[i])) )
      perror("pthread_create");
  }
 
  gettimeofday(&tv1, &tz); // засекаем время
 
  if(nc > 2)
    pthread_barrier_wait(&barr1);
 
  for (CurrentTime = 2; CurrentTime < nc-1; CurrentTime++) 
  {
      pthread_barrier_wait(&barr2);
      writeIntoFile(CurrentTime % 2);
      pthread_barrier_wait(&barr1);     
  }
 
  done = 1;
 
  if(nc > 2)
    pthread_barrier_wait(&barr2); 
  writeIntoFile(CurrentTime % 2);
 
  printf("Time: %d milliseconds\n", timeOfWork());
 
  return 0;
}
