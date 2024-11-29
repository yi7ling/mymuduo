#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {

public:
    ThreadPool(int numThreads)
    : numThread_(numThreads),
      stop_(false)
    {
        for (int i = 0; i < numThreads; i++)
        {
            threads_.emplace_back([this]{
                while (1)
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    condition_.wait(lock, [this]{
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty())
                        return;
                    
                    std::function<void()> func = std::move(tasks_.front());
                    tasks_.pop();
                    lock.unlock();
                    func();
                }
            });
        }
    }

    ~ThreadPool()
    {
        stop_ = true;

        for (auto& t: threads_)
        {
            t.join();
        }
    }

    void addTask(std::function<void()> func)
    {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            tasks_.emplace(std::move(func));
        }
        condition_.notify_one();
    }
private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    int numThread_;
};

void func()
{
    printf("dododododo \n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    printf("end do \n");
}

int main(void)
{
    ThreadPool pool(4);

    for (int i = 0; i < 10; i++)
        pool.addTask(func);
}