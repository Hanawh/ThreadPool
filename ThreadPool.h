#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks; //接受任意原型是 void() 的函数或是函数对象,不带参数且没有返回值
    
    // synchronization
    std::mutex queue_mutex; //实际上是锁 保证任务的添加和移除的互斥性
    //提供了独占所有权的特性——即不支持递归地对 std::mutex 对象上锁，而 std::recursive_lock 则可以递归地对互斥量对象上锁
    std::condition_variable condition; 
    //条件变量提供了两类操作：wait和notify 
    bool stop; //保证获取task的同步性
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {   //获取一个待执行的task
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); }); //wait直到有task
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front()); //取一个task 将传入的值转为右值
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(//packaged_task是任务函数的封装类，通过get_future获取future，然后通过future可以获取函数的返回值(future.get())
            std::bind(std::forward<F>(f), std::forward<Args>(args)...) //forward不改变最初传入的类型的引用类型
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex); //对当前块的语句加锁

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one(); //唤醒一个线程执行
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
<<<<<<< HEAD
    condition.notify_all(); //唤醒所有线程执行
    for(std::thread &worker: workers) 
        worker.join(); //等待任务结束
=======
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
>>>>>>> 9a42ec1329f259a5f4881a291db1dcb8f2ad9040
}

#endif
