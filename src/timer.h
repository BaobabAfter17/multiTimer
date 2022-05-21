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
            int num_timeouts;   // Records # of timeouts happened
            std::chrono::time_point<std::chrono::system_clock> expire_time;
            std::function<void()> callback;

            Timer(int id) : id(id), active(false), num_timeouts(0){};
        };

        struct MultiTimer
        {
            MultiTimer(int num_timers); // Initialize Timers with id equals index in vector
            ~MultiTimer();
            int set(
                int id,
                std::chrono::milliseconds timeout,
                std::function<void()> callback
            );  // Start a timer with certain timeout, callback function and args
            int cancel(int id); // Cancel an active timer
            int reset(
                int id,
                std::chrono::milliseconds timeout,
                std::function<void()> callback
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
    }   // namespace v1

    using namespace v1;
}   // namespace mpcs_51044

#endif