#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <functional>
#include <memory>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

namespace mpcs_51044_final_project {
    namespace v1 {
        struct Timer
        {
            int id;
            bool active;
            int num_timeouts;
            // void *callback_args;
            std::chrono::time_point<std::chrono::system_clock> expire_time;
            std::function<void()> callback;

            Timer(int id) : id(id), active(), num_timeouts(){};
        };

        struct MultiTimer
        {
            MultiTimer(int num_timers);
            ~MultiTimer();
            int set(int id, std::chrono::milliseconds timeout, std::function<void()> callback);
            int cancel(int id);
            int reset(int id, std::chrono::milliseconds timeout, std::function<void()> callback);

        private:
            int num_timers;
            bool active;
            std::vector<Timer> timers;
            std::list<std::unique_ptr<Timer>> active_timers;

            std::mutex mx;
            std::condition_variable cv;
            std::thread worker;

            void thread_func();
        };

        bool timer_comp(std::unique_ptr<Timer> const &t1, std::unique_ptr<Timer> const &t2)
        {
            return t1->expire_time < t2->expire_time;
        }
    }   // namespace v1

    using namespace v1;
}   // namespace mpcs_51044

#endif