#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H

#include <list>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "fun.h"

template<typename T>
class threadpool{
public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T request);
    void process(T pinfo);

    static void init_epollfd(int epollfd);
private:
    static int m_epollfd;
    static void* worker(void* arg);
    void run();

private:
    // the number of thread
    int m_thread_number;
    
    // the pointer of threadpool
    pthread_t* m_threads;

    // the maximum number of requests allowed in the request queue that are waiting to be processed
    int m_max_requests;
    
    // request queue
    std::list<T> m_workqueue;

    // protect the mutex of the request queue
    locker m_queuelocker;

    // are there any tasks thst need to be handled
    sem m_queuestat;

    // end thread or not
    bool m_stop;
};

template<typename T>
void threadpool<T>::init_epollfd(int epollfd){
    m_epollfd = epollfd;
}

template<typename T> int threadpool<T>::m_epollfd = -1;

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):
        m_thread_number(thread_number), m_max_requests(max_requests),
        m_stop(false), m_threads(NULL){
    if((thread_number <= 0) || (max_requests <= 0)){
        throw std::exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }
    
    for(int i = 0; i < m_thread_number; i++){
        printf("create the %dth thread\n", i);
        if(pthread_create(m_threads + i, NULL, worker, this) != 0){
            delete []m_threads;
            throw std::exception();
        }

        if(pthread_detach(m_threads[i])){
            delete []m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool(){
    delete []m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T request){
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg){
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        T request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request){
            continue;
        }
        process(request);
    }
}

template<typename T>
void threadpool<T>::process(T pinfo){

    int temp = pinfo;
    
    // char cliIP[16] = {0};
    // inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, cliIP, sizeof(cliIP));
    // unsigned short cliPort = ntohs(pinfo->addr.sin_port);
    // printf("client ip is: %s, port is %d\n", cliIP, cliPort);

    char recvBuf[1024] = {0};
    setNoBlocking(temp);
    int ret = 0;
    time_t timer;
    struct tm* Now;

    while(read(temp, recvBuf, sizeof(recvBuf)) != 0){
        printf("server recv message: %s\n", recvBuf);
        bzero(recvBuf, 0);

        sleep(1);

        time(&timer);
        Now = localtime(&timer);
        char *res = asctime(Now);
        ret = write(temp, res, strlen(res));
    }
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, temp, NULL);
    close(temp);
}

#endif