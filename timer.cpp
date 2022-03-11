#include <string>
#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <memory>

using namespace std::chrono_literals;

struct Timer
{
    int id;
    bool active;
    int num_timeouts;
    // void *callback_args;
    std::chrono::time_point<std::chrono::system_clock> expire_time;
    std::function<void()> callback;

    Timer(int id) : id(id), active(), num_timeouts() {};
};

bool timer_comp(std::unique_ptr<Timer> const & t1, std::unique_ptr<Timer> const & t2)
{
    return t1->expire_time < t2->expire_time;
}


struct MultiTimer
{
    int num_timers;
    bool active;
    std::vector<Timer> timers;
    std::list<std::unique_ptr<Timer>> active_timers;

    std::mutex mx;
    std::condition_variable cv;
    std::thread worker;

    MultiTimer(int num_timers);
    ~MultiTimer();
    int set(int id, std::chrono::milliseconds timeout, std::function<void()> callback);
    int cancel(int id);
private:
    void thread_func();
};

// num_timers must be positive
MultiTimer::MultiTimer(int num_timers) : num_timers(num_timers)
{
    active = true;
    for (int i = 0; i < num_timers; i++) {
        timers.push_back(Timer{i});
    }
    worker = std::thread(&MultiTimer::thread_func, this);
}

MultiTimer::~MultiTimer()
{
    std::unique_lock<std::mutex> lk(mx);
    active = false;
    cv.notify_one();
    lk.unlock();
    worker.join();
}

void MultiTimer::thread_func()
{
    while (active) {
        std::unique_lock<std::mutex> lk(mx);
        std::cout << "before cv wait" << std::endl;
        if (active_timers.empty()) {
            std::cout << "active timers empty" << std::endl;
            cv.wait(lk, [=]{ return !active_timers.empty(); });
        } else {
            std::cout << "active timers not empty" << std::endl;
            auto closest_expire = active_timers.front()->expire_time;
            cv.wait_until(lk, closest_expire);
            active_timers.front()->num_timeouts++;
            active_timers.front()->active = false;
            active_timers.pop_front();
        }
        lk.unlock();
    }
    std::cout << "# of timers: " << num_timers << std::endl;
    std::cout << "# of active: " << active_timers.size() << std::endl;
}

int MultiTimer::set(int id, std::chrono::milliseconds timeout, std::function<void()> callback)
{
    std::lock_guard lk(mx);
    auto timer_p = std::make_unique<Timer>(timers[id]);
    timer_p->callback = callback;
    timer_p->expire_time = std::chrono::system_clock::now() + timeout;

    timer_p->active = true;
    active_timers.push_back(std::move(timer_p));
    active_timers.sort(timer_comp);
    std::cout << "# of pointers in active_timers: " << active_timers.size() << std::endl;
    cv.notify_one();
    return 0;
}


void print_timeout()
{
    std::cout << "Timeout" << std::endl;
} 

int main()
{
    // Timer t{};
    // t.set(1, 3s, print_timeout);
    // std::this_thread::sleep_for(5s);
    // if (std::chrono::system_clock::now() > t.expire_time)
    //     std::invoke(t.callback);
    // std::cout << "Timer #" << t.id << std::endl;

    MultiTimer t{1};
    t.set(0, 1s, print_timeout);
}