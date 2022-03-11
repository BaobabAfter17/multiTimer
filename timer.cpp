#include <string>
#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <memory>

#define FMT_HEADER_ONLY
#include "lib/fmt/format.h"
#include "timer.h"
#include "log.h"

using namespace std::chrono_literals;
using namespace mpcs_51044_final_project::v1;
using namespace mpcs_51044_log;
using namespace fmt;


template <typename... Args>
MultiTimer<void(Args...)>::MultiTimer(int num_timers)
: num_timers(num_timers)
{
    print_log(INFO, "Start initializing MultiTimer...");
    active = true;
    for (int i = 0; i < num_timers; i++) {
        timers.push_back(Timer{i});
    }
    worker = std::thread(&MultiTimer::thread_func, this);
}

template <typename... Args>
MultiTimer<void(Args...)>::~MultiTimer()
{
    std::unique_lock<std::mutex> lk(mx);
    active = false; // Stop thread_func in worker thread
    cv.notify_one();
    lk.unlock();
    worker.join();
    print_log(INFO, "MultiTimer destoryed...");
}

template <typename... Args>
void MultiTimer<void(Args...)>::thread_func()
{
    while (active) {
        std::unique_lock<std::mutex> lk(mx);
        if (active_timers.empty()) {
            print_log(INFO, format("No active timers, waiting..."));
            cv.wait(lk, [=]
                { return !active_timers.empty(); });
        } else {    // Have some active timers

            // Check the active timer with closest expire time
            auto closest_expire = active_timers.front()->expire_time;
            int current_timer_id = active_timers.front()->id;
            print_log(INFO, format("Timer #{} starting tiking down", current_timer_id));
            cv.wait_until(lk, closest_expire);

            print_log(INFO, format("Timer #{} time out!", current_timer_id));
            active_timers.front()->num_timeouts++;
            active_timers.front()->callback();
            print_log(INFO, "Callback function executed...");

            // Remove expired timer from list; it is still kept in the vector
            active_timers.front()->active = false;
            active_timers.pop_front();
        }
        lk.unlock();
    }
}

template <typename... Args>
int MultiTimer<void(Args...)>::set(
    int id,
    std::chrono::milliseconds timeout,
    std::function<void(Args...)> callback,
    Args&&...args
){
    // Invalid id or timer already active
    if (id < 0 || id >= num_timers || timers[id].active)
        return 1;

    std::lock_guard lk(mx);
    auto timer_p = std::make_unique<Timer>(timers[id]);
    timer_p->callback = std::bind(callback, std::forward<Args>(args)...);
    timer_p->expire_time = std::chrono::system_clock::now() + timeout;

    timer_p->active = true;
    active_timers.push_back(std::move(timer_p));
    active_timers.sort(timer_comp);
    print_log(INFO, format("Timer #{} set!", id));
    cv.notify_one();
    return 0;
}

template <typename... Args>
int MultiTimer<void(Args...)>::cancel(int id)
{
    if (id < 0 || id >= num_timers || !timers[id].active)
        return 1;

    std::lock_guard lk(mx);
    timers[id].active = false;
    active_timers.remove_if([=](std::unique_ptr<Timer> const &p)
                 { return p->id == id; });
    print_log(INFO, format("Timer #{} cancelled!", id));
    cv.notify_one();
    return 0;
}

template <typename... Args>
int MultiTimer<void(Args...)>::reset(
    int id,
    std::chrono::milliseconds timeout,
    std::function<void(Args...)> callback,
    Args&&... args
){
    int status;
    if ((status = cancel(id)) != 0)
        return status;
    if ((status = set(id, timeout, callback, args...)) != 0)
        return status;
    print_log(INFO, format("Timer #{} reset successful!", id));
    return 0;
}


// TEST
void print_timeout(int i)
{
    std::cout << "This is the callback function" << std::endl;
    std::cout << "Int i is " << i << std::endl;
} 

int main()
{
    MultiTimer<void(int)> t{1};
    t.set(0, 5s, print_timeout, 23);
    std::this_thread::sleep_for(1s);
    t.cancel(0);
}