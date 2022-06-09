# MultiTimer in a Separate Thread

# Backgrounds
This project is inspired by the timers in TCP (Transmission Control Protocol). However, this project is totally independent.

The timer in TCP is created with a timeout (say 10 seconds) and a callback function (say it's foo) in the main thread. The timer runs **in a separate thread** and starts ticking once set. Normally the timer will expire after a certain time (10 seconds) and runs the callback function foo. However, if the main thread signals the timer before the expiration time (say 5 seconds after timer was set),  the timer will reset countdown time to 10 seconds.

This project realizes multiple (say 100) timers in a separate single thread. I add the following features on top of laster quarter's final project:
1. File organization. Add CMake support and make all source files more organized.
2. Add frontend demo. The connection between frontend and backend server is built using `boost::beast::websocket` and `boot::asio::ip::tcp`.
3. Revised original code. Add `std::regex` and other optimizations.


# How to run
0. This project requires `boost` library installed on your computer.
1. Compile source files and start the server.
```
mkdir build
cd build
cmake ..
make
./server
```
2. Open another terminal, go to root directory(`cd ..` from `build/` folder) and start a http server.
```
python3 -m http.server
```
3. Open a web browser and go to `localhost:8000`.
4. Click around on the web page. There are five timers in the MultiTimer, all run in the same thread. Click `Set`, `Cancel` or `Reset` button to see its status change below.


# Main C++ techniques used

## `log.h`
1. `enum`
2. Overloading `operator<<`
3. `string_view`
4. Avoid multiple inclusions with `#ifndef`

## `timer.h`
1. Namespace versioning
2. Template: general template, partial speciliazation
3. `std::function`, `std::chrono`
4. Class constructor & destructor
5. Universal reference in class template `Args&&...`
6. `std::vector`, `std::list` and their APIs
7. `mutex`, `conditional_variable`, `thread`
8. `unique_ptr`, RIGHT `const`

## `timer.cpp`
1. `std::chrono_literals`, time points, durations
2. `{fmt}` (C++20 `<format>`)
3. `this`
4. `unique_lock` and `lock_guard`
5. `std::conditional_variable::wait`, `std::conditional_variable::wait_until`
6. `cv.notify_one()`
7. Lambda functions, capture lists
8. `std::bind`
9. `std::forward`
10. `std::move`
11. Pass by reference & pass by value

## `server.cpp`
1. `std::regex`