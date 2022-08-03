#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "wrap.h"

#define DEFAULT_TIME 1                 /*1s检测一次*/
#define MIN_WAIT_TASK_NUM 2            /*如果queue_size > MIN_WAIT_TASK_NUM 添加新的线程到线程池*/ 
#define MAX_EVENTS  4096               //epoll监听上限数
#define BUFLEN 4096			

#define DEFAULT_THREAD_VARY 5          /*每次创建和销毁线程的个数*/
#define true 1
#define false 0

#define MAXLINE2 4096
#define SERV_PORT2 7777

void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

typedef struct {
    void *(*function)(void *);          /* 函数指针，回调函数 */
    void *arg;                          /* 上面函数的参数 */

} threadpool_task_t;                    /* 各子线程任务结构体 */

/* 描述线程池相关信息 */

typedef struct threadpool_t {
    pthread_mutex_t lock;               /* 用于锁住本结构体 */    
    pthread_mutex_t thread_counter;     /* 记录忙状态线程个数de琐 -- busy_thr_num */

    pthread_cond_t queue_not_full;      /* 当任务队列满时，添加任务的线程阻塞，等待此条件变量 */
    pthread_cond_t queue_not_empty;     /* 任务队列里不为空时，通知等待任务的线程 */

    pthread_t *threads;                 /* 存放线程池中每个线程的tid。数组 */
    pthread_t adjust_tid;               /* 存管理线程tid */
    threadpool_task_t *task_queue;      /* 任务队列(数组首地址) */

    int min_thr_num;                    /* 线程池最小线程数 */
    int max_thr_num;                    /* 线程池最大线程数 */
    int live_thr_num;                   /* 当前存活线程个数 */
    int busy_thr_num;                   /* 忙状态线程个数 */
    int wait_exit_thr_num;              /* 要销毁的线程个数 */

    int queue_front;                    /* task_queue队头下标 */
    int queue_rear;                     /* task_queue队尾下标 */
    int queue_size;                     /* task_queue队中实际任务数 */
    int queue_max_size;                 /* task_queue队列可容纳任务数上限 */

    int shutdown;                       /* 标志位，线程池使用状态，true或false */
}threadpool_t;

void *threadpool_thread(void *threadpool);
int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), void *arg);

void *adjust_thread(void *threadpool);

int is_thread_alive(pthread_t tid);
int threadpool_free(threadpool_t *pool);


/* 描述就绪文件描述符相关信息 */

struct myevent_s {
    int fd;                                                 //要监听的文件描述符
    int events;                                             //对应的监听事件
    void *arg;                                              //泛型参数
    void (*call_back)(int fd, int events, void *arg);       //回调函数
    int status;                                             //是否在监听:1->在红黑树上(监听), 0->不在(不监听)
    char buf[BUFLEN];
    int len;
    long last_active;                                       //记录每次加入红黑树 g_efd 的时间值
    int thread_pos;											//记录全局数组 struct s_info ts[256] 的下标供线程使用 
    threadpool_t *thp;

};


struct s_info {				/* thread action args */
	struct sockaddr_in cliaddr;
	int connfd;
	
	int fd;								/*              */ 
	int events;							// 这三个参数用于将epoll监听红黑树上的回调函数和线程池的回调函数连接起来 
	void *arg;							/*              */
};




int g_efd;                                                  //全局变量, 保存epoll_create返回的文件描述符
struct myevent_s g_events[MAX_EVENTS+1];                    //自定义结构体类型数组. +1-->listen fd
struct s_info ts[4096]; 										//用于记录线程 
int k = 0; 
/*  ev.thread_pos = k;								       另外启用一个线程准备work 
	k = (k+1)%4096;	
	这个k还是要上一个锁		
*/ 
pthread_mutex_t klock;




/*将结构体 myevent_s 成员变量 初始化*/
/* eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]); */
void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg)
{
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    memset(ev->buf, 0, sizeof(ev->buf));
    ev->len = 0;
    ev->last_active = time(NULL);                       //调用eventset函数的时间

    return;
}

/* 向 epoll监听的红黑树 添加一个 文件描述符 */
/* eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]); */
void eventadd(int efd, int events, struct myevent_s *ev)
{
	/* 从自定义的结构体指针struct myevent_s *的变量ev中 提取数据到一个可以挂在到epoll监听红黑树上的struct epoll_event变量 epv上 */
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;
//    epv.events = ev->events = events;       //EPOLLIN 或 EPOLLOUT
    epv.events = EPOLLIN | EPOLLET;
    if (ev->status == 0) {                                          //已经在红黑树 g_efd 里
        op = EPOLL_CTL_ADD;                 //将其加入红黑树 g_efd, 并将status置1
        ev->status = 1;
    }

    if (epoll_ctl(efd, op, ev->fd, &epv) < 0)                       //实际添加/修改
        printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
    else
        printf("event add OK [fd=%d], op=%d, events[%0X]\n", ev->fd, op, events);

    return ;
}

/* 从epoll 监听的 红黑树中删除一个 文件描述符*/

void eventdel(int efd, struct myevent_s *ev)
{
    struct epoll_event epv = {0, {0}};

    if (ev->status != 1)                                        //不在红黑树上
        return ;

    //epv.data.ptr = ev;
    epv.data.ptr = NULL;										//抹去指针
    ev->status = 0;                                             //修改状态
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);                //从红黑树 efd 上将 ev->fd 摘除

    return ;
}

/*  当有文件描述符就绪, epoll返回, 调用该函数 与客户端建立链接 */


/* 在acceptconn内部去做accept */
void acceptconn(int lfd, int events, void *arg)
{
    puts("acception running...\n");
    struct sockaddr_in cin;
    socklen_t len = sizeof(cin);
    int cfd, i;

    if ((cfd = accept(lfd, (struct sockaddr *)&cin, &len)) == -1) {
        if (errno != EAGAIN && errno != EINTR) {
            /* 暂时不做出错处理 */
        }
        printf("%s: accept, %s\n", __func__, strerror(errno));
        return ;
    }

    do {
        for (i = 0; i < MAX_EVENTS; i++)                                //从全局数组g_events中找一个空闲元素
            if (g_events[i].status == 0)                                //类似于select中找值为-1的元素
                break;                                                  //跳出 for

        if (i == MAX_EVENTS) {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;                                                      //跳出do while(0) 不执行后续代码
        }

        int flag = 0;
        if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0) {             //将cfd也设置为非阻塞
            printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
            break;
        }
        pthread_mutex_lock(&klock);
        g_events[i].thread_pos = k;
        k = (k+1)%4096;		/*  k作为参数传递  */
	pthread_mutex_unlock(&klock);
        ts[g_events[i].thread_pos].cliaddr = cin;
		ts[g_events[i].thread_pos].connfd = cfd;

        /* 给cfd设置一个 myevent_s 结构体, 回调函数 设置为 recvdata */
        eventset(&g_events[i], cfd, recvdata, &g_events[i]);		
        eventadd(g_efd, EPOLLIN, &g_events[i]);                         //将cfd添加到红黑树g_efd中,监听读事件
        
		//思路：把任务添加函数threadpool_add封装进结构体的回调函数 recvdata 中 
    } while(0);

    printf("new connect [%s:%d][time:%ld], pos[%d]\n",
            inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i);
    return ;
}

void *do_rw(void *arg);


void recvdata(int fd, int events, void *arg){
	
	struct myevent_s *ev = (struct myevent_s *)arg;
	
	int position =  ev->thread_pos; 
	
	ts[position].arg = arg;
	ts[position].events = events;
	ts[position].fd = fd;
	threadpool_t *thp = ev->thp;
	threadpool_add(thp, do_rw, (void*)&ts[position]);   /* 向线程池中添加任务 */	

}
	
	/* 线程池中的线程，模拟处理业务 */
void *do_rw(void *arg){

	puts("do_rw is trigger!\n");
	int i;
	struct s_info *ts = (struct s_info *)arg;
	int fd = ts->fd; 
	int events = ts->events; 
	void *argg = ts->arg;
	
	struct myevent_s *ev = (struct myevent_s *)argg;
	
	char buf[MAXLINE2];
	char str[INET_ADDRSTRLEN];
	/* 可以在创建线程前设置线程创建属性,设为分离态 */
	pthread_detach(pthread_self());
	printf("(rw)thread %d is recieving from PORT %d\n", (unsigned int)pthread_self(), ntohs((*ts).cliaddr.sin_port));


	while (1) {
		int n = Read(fd, buf, MAXLINE2);
		if (n == 0) {		//因为在socket中的读也是阻塞的
			printf("the other side has been closed.\n");
			break;
		}else if(n == -1){
			continue;
		}
		printf("(rw)thread %d  received from %s at PORT %d\n", (unsigned int)pthread_self(),
				inet_ntop(AF_INET, &(*ts).cliaddr.sin_addr, str, sizeof(str)),
				ntohs((*ts).cliaddr.sin_port));
		for (i = 0; i < n; i++)
			buf[i] = toupper(buf[i]);
		Write(ts->connfd, buf, n);
	}
	close(ts->connfd);
	
	//做完任务之后要把事件从树上摘下 
	eventdel(fd, ev);
    pthread_exit(NULL);
}



//threadpool_create(2,4096,4096);  
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
    int i;
    threadpool_t *pool = NULL;          /* 线程池 结构体 */

    do {
        if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {  
            printf("malloc threadpool fail");
            break;                                      /*跳出do while*/
        }

        pool->min_thr_num = min_thr_num;
        pool->max_thr_num = max_thr_num;
        pool->busy_thr_num = 0;
        pool->live_thr_num = min_thr_num;               /* 活着的线程数 初值=最小线程数 */
        pool->wait_exit_thr_num = 0;
        pool->queue_size = 0;                           /* 有0个产品 */
        pool->queue_max_size = queue_max_size;          /* 最大任务队列数 */
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = false;                         /* 不关闭线程池 */

        /* 根据最大线程上限数， 给工作线程数组开辟空间, 并清零 */
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t)*max_thr_num); 
        if (pool->threads == NULL) {
            printf("malloc threads fail");
            break;
        }
        memset(pool->threads, 0, sizeof(pthread_t)*max_thr_num);

        /* 给 任务队列 开辟空间 */
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t)*queue_max_size);
        if (pool->task_queue == NULL) {
            printf("malloc task_queue fail");
            break;
        }

        /* 初始化互斥琐、条件变量 */
        if (pthread_mutex_init(&(pool->lock), NULL) != 0
                || pthread_mutex_init(&(pool->thread_counter), NULL) != 0
                || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0
                || pthread_cond_init(&(pool->queue_not_full), NULL) != 0)
        {
            printf("init the lock or cond fail");
            break;
        }

        /* 启动 min_thr_num 个 work thread */
        for (i = 0; i < min_thr_num; i++) {
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);   /*pool指向当前线程池,作为线程回调函数的参数 void* args*/
            printf("start thread %d...\n", (unsigned int)pool->threads[i]);
        }
        
        /*  创建 1 个管理者线程。 */
        pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);     /* 创建管理者线程 */

        return pool;

    } while (0);

    threadpool_free(pool);      /* 前面代码调用失败时，释放poll存储空间 */

    return NULL;
}

/* 向线程池中 添加一个任务 */
//threadpool_add(thp, process, (void*)&num[i]);   /* 向线程池中添加任务 process: 小写---->大写*/
/* 在这个函数里面把任务写道任务队列中 */ 
int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), void *arg)
{ 
	/*先对线程池结构体本身上锁*/ 
    pthread_mutex_lock(&(pool->lock));

    /* ==为真，队列已经满， 调wait阻塞 */
    while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)) {
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }

    if (pool->shutdown) {
        pthread_cond_broadcast(&(pool->queue_not_empty));
        pthread_mutex_unlock(&(pool->lock));
        return 0;
    }

    /* 清空 工作线程 调用的回调函数 的参数arg */
    if (pool->task_queue[pool->queue_rear].arg != NULL) {
        pool->task_queue[pool->queue_rear].arg = NULL;
    }

    /*添加任务到任务队列里*/
    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;       /* 队尾指针移动, 模拟环形 */
    pool->queue_size++;

    /*添加完任务后，队列不为空，唤醒线程池中 等待处理任务的线程*/
    pthread_cond_signal(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

/* 线程池中各个工作线程 */
void *threadpool_thread(void *threadpool)/* 参数是指向线程池结构体的指针  */ 
{
    threadpool_t *pool = (threadpool_t *)threadpool; /* 还原出线程池结构体pool */ 
    threadpool_task_t task;

    while (true) {
        /* Lock must be taken to wait on conditional variable */
        /*刚创建出线程，等待任务队列里有任务，否则阻塞等待任务队列里有任务后再唤醒接收任务*/
        
        
        /* 在线程访问公用的线程池结构体pool 之前先上互斥锁*/
        pthread_mutex_lock(&(pool->lock));

        /*queue_size == 0 说明没有任务，调 wait 阻塞在条件变量上, 若有任务，跳过该while*/
        while ((pool->queue_size == 0) && (!pool->shutdown)) {  
            printf("thread %d is waiting\n", (unsigned int)pthread_self());
            
            /* 阻塞等待条件变量queue_not_empty 并解开 lock互斥锁，直到条件变量满足则上lock锁并执行  */ 
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));

            /*清除指定数目的空闲线程，如果要结束的线程个数大于0，结束线程*/
            /* wait_exit_thr_num 表示要销毁的线程 */ 
            if (pool->wait_exit_thr_num > 0) {
                pool->wait_exit_thr_num--;

                /*如果线程池里线程个数大于最小值时可以结束当前线程*/
                /* 注意该if是在上一个if里的 */
                /* live_thr_num表示当前存活的线程个数 */
                if (pool->live_thr_num > pool->min_thr_num) {
                    printf("thread %d is exiting\n", (unsigned int)pthread_self());
                    pool->live_thr_num--;
                    
                    
                    /* 在解锁之后退出 */ 
                    /* 使用pthread_mutex的颗粒度尽量小；
					* 特别注意while，如果解锁写在while的最尾，上锁写在while的最前，
					* 那么其他线程几乎不可能抢到这把锁 */ 
                    pthread_mutex_unlock(&(pool->lock));

					/* 处在threadpool_thread回调函数中，若这里exit之后的代码就不会执行了 */
                    pthread_exit(NULL);
                }
            }
        }

        /*如果指定了true，要关闭线程池里的每个线程，自行退出处理---销毁线程池*/
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
          //  printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
          printf("线程退出\n");
            pthread_detach(pthread_self());  /* 将自己分离，资源被自动回收，主要是回收资源 */ 
            pthread_exit(NULL);     /* detach之后线程并没有死，还需要pthread_exit()使得线程自行结束 */
        }

        /*从任务队列里获取任务, 是一个出队操作*/
        task.function = pool->task_queue[pool->queue_front].function; //取回调函数 
        task.arg = pool->task_queue[pool->queue_front].arg;  //取回调函数的参数 

        pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;       /* 出队，模拟环形队列 */
        pool->queue_size--;

        /*通知可以有新的任务添加进来*/
        /* pthread_cond_broadcast()： 唤醒阻塞在条件变量上的 所有线程 */
	printf("thread %d toke one task, new task can be added!\n", (unsigned int)pthread_self());
        pthread_cond_broadcast(&(pool->queue_not_full));

        /*任务取出后到临时变量task中之后，立即将 线程池的互斥锁lock 释放*/
        pthread_mutex_unlock(&(pool->lock));

        /*执行任务*/ 
        printf("thread %d start working\n", (unsigned int)pthread_self());
        
        
        /* thread_counter是专门管理忙线程个数（busy_thr_num）的互斥锁 */
        pthread_mutex_lock(&(pool->thread_counter));                            /*忙状态线程数变量琐*/
        pool->busy_thr_num++;                                                   /*忙状态线程数+1*/
        pthread_mutex_unlock(&(pool->thread_counter));


		/*threadpool_thread本身也属于一个回调函数，这波属于回调函数调用另一个回调函数*/
        (*(task.function))(task.arg);                                           /*执行回调函数任务*/
        //task.function(task.arg);                                              /*执行回调函数任务*/


        /*任务结束处理*/ 
        printf("thread %d end working\n", (unsigned int)pthread_self());
        
         /*处理掉一个任务，忙状态数线程数-1*/
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num--;                                       /*处理掉一个任务，忙状态数线程数-1*/
        pthread_mutex_unlock(&(pool->thread_counter));
    }

    pthread_exit(NULL);
}

/* 管理线程 */
void *adjust_thread(void *threadpool)
{
    int i;
    threadpool_t *pool = (threadpool_t *)threadpool;
    while (!pool->shutdown) {

        sleep(1);                                    /*定时 对线程池管理*/

        pthread_mutex_lock(&(pool->lock));
        
        int queue_size = pool->queue_size;                      /* 关注 任务数 */
        
        int live_thr_num = pool->live_thr_num;                  /* 存活 线程数 */
        
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        int busy_thr_num = pool->busy_thr_num;                  /* 忙着的线程数 */
        pthread_mutex_unlock(&(pool->thread_counter));

        /* 1. 创建新线程 算法： 任务数大于最小线程池个数, 且存活的线程数少于最大线程个数时 如：30>=10 && 40<100*/
        if (queue_size >= MIN_WAIT_TASK_NUM && live_thr_num < pool->max_thr_num) {
     	pthread_mutex_lock(&(pool->lock));  
            int add = 0;

            /*一次增加 DEFAULT_THREAD 个线程*/
            for (i = 0; i < pool->max_thr_num && add < DEFAULT_THREAD_VARY
                    && pool->live_thr_num < pool->max_thr_num; i++) {
                if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i])) {
                    pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
                    add++;
                    pool->live_thr_num++;
                }
            }

            pthread_mutex_unlock(&(pool->lock));
        }

        /* 2.  销毁多余的空闲线程 算法：忙线程X2 小于 存活的线程数 且 存活的线程数 大于 最小线程数时*/
        if ((busy_thr_num * 2) < live_thr_num  &&  live_thr_num > pool->min_thr_num) {

            /* 一次销毁DEFAULT_THREAD个线程, 隨機5個即可 */
            pthread_mutex_lock(&(pool->lock));
            pool->wait_exit_thr_num = DEFAULT_THREAD_VARY;      /* 要销毁的线程数 设置为5 */
            pthread_mutex_unlock(&(pool->lock));

            for (i = 0; i < DEFAULT_THREAD_VARY; i++) {
                /* 通知处在空闲状态的线程, 他们会自行终止*/
                /* 所谓通知处在空闲状态的线程，就是唤醒阻塞在queue_not_empty条件变量上的线程 */ 
                /* pthread_cond_signal()的功能是一次唤醒一个进程 */
                pthread_cond_signal(&(pool->queue_not_empty));
            }
        }
    }
    return NULL;
}

int threadpool_destroy(threadpool_t *pool)
{
    int i;
    if (pool == NULL) {
        return -1;
    }
    pool->shutdown = true;

    /*先销毁管理线程*/
    pthread_join(pool->adjust_tid, NULL);

    for (i = 0; i < pool->live_thr_num; i++) {
        /*通知所有的空闲线程*/
        pthread_cond_broadcast(&(pool->queue_not_empty));
    }
    for (i = 0; i < pool->live_thr_num; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    threadpool_free(pool);

    return 0;
}

int threadpool_free(threadpool_t *pool)
{
    if (pool == NULL) {
        return -1;
    }

    if (pool->task_queue) {
        free(pool->task_queue);
    }
    if (pool->threads) {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));
        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));
    }
    free(pool);
    pool = NULL;

    return 0;
}

int threadpool_all_threadnum(threadpool_t *pool)
{
    int all_threadnum = -1;                 // 总线程数

    pthread_mutex_lock(&(pool->lock));
    all_threadnum = pool->live_thr_num;     // 存活线程数
    pthread_mutex_unlock(&(pool->lock));

    return all_threadnum;
}

int threadpool_busy_threadnum(threadpool_t *pool)
{
    int busy_threadnum = -1;                // 忙线程数

    pthread_mutex_lock(&(pool->thread_counter));
    busy_threadnum = pool->busy_thr_num;    
    pthread_mutex_unlock(&(pool->thread_counter));

    return busy_threadnum;
}

int is_thread_alive(pthread_t tid)
{
    int kill_rc = pthread_kill(tid, 0);     //发0号信号，测试线程是否存活
    if (kill_rc == ESRCH) {
        return false;
    }
    return true;
}




int main(void){
	pthread_mutex_init(&klock,NULL);
	threadpool_t *thp = threadpool_create(10,4096,4096);   /*创建线程池，池里最小n1个线程，最大n2，队列最大n3*/

    // int j = 0;
    // for(; j<MAX_EVENTS+1; j++ ){
	// g_events[j].thp = thp;
    // }

    // printf("pool inited");
    // pthread_t tid;
    
    // struct sockaddr_in servaddr, cliaddr;
    // socklen_t cliaddr_len;
	
	// int listenfd, connfd;
    
    // listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    // fcntl(listenfd, F_SETFL, O_NONBLOCK);                                            //将socket设为非阻塞
    
    // bzero(&servaddr, sizeof(servaddr));
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // servaddr.sin_port = htons(SERV_PORT2);
    
    // Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    // Listen(listenfd, 128);
    
    // printf("\nThread pool start accepting connections ...\n");
    
    // /* void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg);  */
    // eventset(&g_events[MAX_EVENTS], listenfd, acceptconn, &g_events[MAX_EVENTS]);
    
    // g_efd = epoll_create(MAX_EVENTS+1);                 //创建红黑树,返回给全局 g_efd 
    // if (g_efd <= 0)
    //     printf("create efd in %s err %s\n", __func__, strerror(errno));

    // /* void eventadd(int efd, int events, struct myevent_s *ev) */
    // eventadd(g_efd, EPOLLIN, &g_events[MAX_EVENTS]);
    
    // struct epoll_event events[MAX_EVENTS+1];            //保存已经满足就绪事件的文件描述符数组 以供epoll_wait使用 
    // int checkpos = 0, i;
    // while (1) {
    //     /* 超时验证，每次测试100个链接，不测试listenfd 当客户端60秒内没有和服务器通信，则关闭此客户端链接 */

    //     long now = time(NULL);                          //当前时间
    //     for (i = 0; i < 100; i++, checkpos++) {         //一次循环检测100个。 使用checkpos控制检测对象
    //         if (checkpos == MAX_EVENTS)					//根节点不参与检测 
    //             checkpos = 0;
    //         if (g_events[checkpos].status != 1)         //不在红黑树 g_efd 上
    //             continue;

    //         long duration = now - g_events[checkpos].last_active;       //客户端不活跃的世间

    //         if (duration >= 60) {
    //             close(g_events[checkpos].fd);                           //关闭与该客户端链接
    //             printf("[fd=%d] timeout\n", g_events[checkpos].fd);
    //             eventdel(g_efd, &g_events[checkpos]);                   //将该客户端 从红黑树 g_efd移除
    //         }
    //     }

    //     /*监听红黑树g_efd, 将满足的事件的文件描述符加至events数组中, 1秒没有事件满足, 返回 0*/
    //     int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, -1);
    //     if (nfd < 0) {
    //         printf("epoll_wait error, exit\n");
    //         break;
    //     }

    //     for (i = 0; i < nfd; i++) {
    //         /*使用自定义结构体myevent_s类型指针, 接收 联合体data的void *ptr成员*/
    //         struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;  

    //         if (events[i].events & (EPOLLIN | EPOLLET)) {           //读就绪事件
    //    		if(events[i].data.fd == listenfd) continue;      
	// 	    ev->call_back(ev->fd, events[i].events, ev->arg);
    //         }
            
    //     }
    // }

    /* 退出前释放所有资源 */
    puts("About exit in 100 Seconds...");
    //sleep(100);                                          /* 等子线程完成任务 */
    threadpool_destroy(thp);
    sleep(5);
    return 0;
    
} 
