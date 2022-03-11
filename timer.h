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
        template <typename T> struct MultiTimer;

        struct Timer
        {
            int id;
            bool active;
            int num_timeouts;   // Records # of timeouts happened
            std::chrono::time_point<std::chrono::system_clock> expire_time;
            std::function<void()> callback;

            Timer(int id) : id(id), active(), num_timeouts(){};
        };

        template <typename... Args>
        struct MultiTimer<void(Args...)>
        {
            MultiTimer(int num_timers); // Initialize Timers with id equals index in vector
            ~MultiTimer();
            int set(
                int id,
                std::chrono::milliseconds timeout,
                std::function<void(Args...)> callback,
                Args&&... args
            );  // Start a timer with certain timeout, callback function and args
            int cancel(int id); // Cancel an active timer
            int reset(
                int id,
                std::chrono::milliseconds timeout,
                std::function<void(Args...)> callback,
                Args&&... args
            );  // Cancel an active timer and restart it

        private:
            int num_timers; // # of Timers in this MultiTimer
            bool active;
            std::vector<Timer> timers;  // All Timers, active or not
            // A sorted list of active timers; sorted by expire_time
            std::list<std::unique_ptr<Timer>> active_timers;

            // Used to control worker thread
            std::mutex mx;
            std::condition_variable cv;
            std::thread worker; // Timers run in this thread

            void thread_func(); // Call by worker thread
        };

        // Compare Timers according to their expire time
        bool timer_comp(std::unique_ptr<Timer> const &t1, std::unique_ptr<Timer> const &t2)
        {
            return t1->expire_time < t2->expire_time;
        }
    }   // namespace v1

    using namespace v1;
}   // namespace mpcs_51044

#endif