/*
* @file - This code performs the latency check where a GPIO interrupt when triggered would release a
          semaphore to a higher priority thread which would inturn toggle the LED.
* @authors - Deekshith Reddy Patil and Jahnavi Pinnamaneni  
*/

/* Header file initialization */
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <semaphore.h>

#include <syslog.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <fcntl.h>

#define LED				0
#define IN				2


void print_scheduler(void);

/*
* @desc - this function waits on a semaphore to toggle an LED
*/
void *Service_1(void *threadp);
sem_t semS1;

/* 
* @desc - GPIO Interrupt Handler
*/
void myInterrupt()
{
  sem_post(&semS1);	
}

/* Application Entry Point */
int main (void)
{
	 int i, rc, scope;
	 pthread_attr_t main_attr;
	 struct sched_param main_param;
	 pid_t mainpid;
	 int rt_max_prio, rt_min_prio;
	 pthread_attr_t rt_sched_attr;
	 struct sched_param rt_param;
	 pthread_t threads;
	  wiringPiSetup () ;
	  
	  pinMode (LED, OUTPUT) ;
	  wiringPiISR(IN, INT_EDGE_RISING, &myInterrupt);
	  
	  // initialize the sequencer semaphores
	  if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
  
  
      mainpid=getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");
    
    print_scheduler();
    pthread_attr_getscope(&main_attr, &scope);
    rc=pthread_attr_init(&rt_sched_attr);
    rc=pthread_attr_setinheritsched(&rt_sched_attr, PTHREAD_EXPLICIT_SCHED);
    rc=pthread_attr_setschedpolicy(&rt_sched_attr, SCHED_FIFO);
    rt_param.sched_priority=rt_max_prio;
    pthread_attr_setschedparam(&rt_sched_attr, &rt_param);
   rc=pthread_create(&threads,               // pointer to thread descriptor
                      &rt_sched_attr,         // use specific attributes
                      //(void *)0,               // default attributes
                      Service_1,                 // thread function entry point
                      NULL // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for service 1");
    else
        printf("pthread_create successful for service 1\n");
    
   digitalWrite (LED, HIGH) ;
   
   while(1)
   {
     int fd = open("temp.txt",O_CREAT | O_TRUNC | O_RDWR);
     
     close(fd);
     
   }
   pthread_join(threads, NULL);
  return 0 ;
}

void *Service_1(void *threadp)
{
    sem_wait(&semS1);
    digitalWrite(LED,LOW);

    pthread_exit((void *)0);
}

void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
       case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
       case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n"); exit(-1);
         break;
       case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n"); exit(-1);
           break;
       default:
           printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }
}
