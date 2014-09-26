#include <iostream>
#include <vector>
#include <chrono>
#include "ThreadPool.h"
int main()
{
    
    ThreadPool pool(4); //创建四个线程的线程池
    std::vector< std::future<int> > results; 
    //之前依靠全局变量来从线程中返回异步任务结果
    //future对象提供访问异步操作结果的机制

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "hello " << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << std::endl;
                return i*i;
            })
        );
    }

    for(auto && result: results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;
    return 0;
}
