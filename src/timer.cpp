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
#include "fmt/format.h"
#include "timer.h"
#include "log.h"

using namespace std::chrono_literals;
using namespace mpcs_51044_final_project::v1;
using namespace mpcs_51044_log;
using namespace fmt;

// Compare Timers according to their expire time
bool timer_comp(std::unique_ptr<Timer> const &t1, std::unique_ptr<Timer> const &t2)
{
    return t1->expire_time < t2->expire_time;
}

MultiTimer::MultiTimer(int num_timers)
: num_timers(num_timers)
{
    print_log(INFO, "Start initializing MultiTimer...");
    active = true;
    for (int i = 0; i < num_timers; i++) {
        timers.push_back(Timer{i});
    }
    worker = std::thread(&MultiTimer::thread_func, this);
}

MultiTimer::~MultiTimer()
{
    std::unique_lock<std::mutex> lk(mx);
    active = false; // Stop thread_func in worker thread
    cv.notify_one();
    lk.unlock();
    worker.join();
    print_log(INFO, "MultiTimer destoryed...");
}

void MultiTimer::thread_func()
{
    while (active) {
        std::unique_lock<std::mutex> lk(mx);
        if (active_timers.empty()) {
            print_log(INFO, format("No active timers, waiting..."));
            cv.wait(lk, [&]
                { return !active_timers.empty(); });
        } else {    // Have some active timers

            // Check the active timer with closest expire time
            print_log(INFO, format("Timer #{} starting tiking down", active_timers.front()->id));
            cv.wait_until(lk, active_timers.front()->expire_time, [&] {
                return active_timers.empty();
            });

            if (active_timers.empty())
                continue;
            if (active_timers.front()->expire_time <= std::chrono::system_clock::now()) {
                // print_log(INFO, format("Timer #{} time out!", active_timers.front()->id));
                active_timers.front()->num_timeouts++;
                active_timers.front()->callback();
                print_log(INFO, "Callback function executed...");

                // Remove expired timer from list; it is still kept in the vector
                active_timers.front()->active = false;
                timers[active_timers.front()->id] = false;
                active_timers.pop_front();
            }
        }
        lk.unlock();
    }
}

int MultiTimer::set(
    int id,
    std::chrono::milliseconds timeout,
    std::function<void()> callback
){
    // Invalid id or timer already active
    if (id < 0 || id >= num_timers || timers[id].active) {
        print_log(INFO, format("Timer #{} already active!", id));
        return 1;
    }

    std::lock_guard lk(mx);
    auto timer_p = std::make_unique<Timer>(timers[id]);
    timer_p->callback = callback;
    timer_p->expire_time = std::chrono::system_clock::now() + timeout;

    timer_p->active = true;
    timers[id].active = true;   // QUESTION: do know why above line is invalid and had to activate timer in this way
    active_timers.push_back(std::move(timer_p));
    active_timers.sort(timer_comp);
    print_log(INFO, format("Timer #{} set!", id));
    cv.notify_one();
    return 0;
}

int MultiTimer::cancel(int id)
{
    if (id < 0 || id >= num_timers || !timers[id].active)
        return 1;

    std::lock_guard lk(mx);
    timers[id].active = false;
    active_timers.remove_if([=](std::unique_ptr<Timer> &p)
                            { return p->id == id; });
    print_log(INFO, format("Timer #{} cancelled!", id));

    cv.notify_one();
    return 0;
}

int MultiTimer::reset(
    int id,
    std::chrono::milliseconds timeout,
    std::function<void()> callback
){
    cancel(id);
    std::this_thread::sleep_for(100ms);
    set(id, timeout, callback);
    print_log(INFO, format("Timer #{} reset successful!", id));
    return 0;
}

// int main() {
//     print_log(INFO, "hello, world\n");
//     return 0;
// }