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
            cv.wait(lk, [&]
                { return !active_timers.empty(); });
        } else {    // Have some active timers

            // Check the active timer with closest expire time
            auto closest_expire = active_timers.front()->expire_time;
            int current_timer_id = active_timers.front()->id;
            print_log(INFO, format("Timer #{} starting tiking down", current_timer_id));
            cv.wait_until(lk, closest_expire);

            if (active_timers.empty())
                continue;
            print_log(DEBUG, format("# timers {}", timers.size()));
            print_log(DEBUG, format("# active timers {}", active_timers.size()));
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
    timers[id].active = true;   // QUESTION: do know why above line is invalid and had to activate timer in this way
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
    active_timers.remove_if([=](std::unique_ptr<Timer> &p)
                            { return p->id == id; });
    print_log(INFO, format("Timer #{} cancelled!", id));
    print_log(DEBUG, format("# active timers {}", active_timers.size()));

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
    cancel(id);
    std::this_thread::sleep_for(100ms);
    set(id, timeout, callback, args...);
    print_log(INFO, format("Timer #{} reset successful!", id));
    return 0;
}


// TEST
// void print_timeout(int i)
// {
//     std::cout << "This is the callback function" << std::endl;
//     std::cout << "Int i is " << i << std::endl;
// } 

// int main()
// {
//     MultiTimer<void(int)> t{1};
//     t.set(0, 5s, print_timeout, 23);
//     std::this_thread::sleep_for(0.5s);
//     t.cancel(0);
// }

// HTML 5 GUI Demo
// Copyright (c) 2019 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "boost/beast/core.hpp"
#include "boost/beast/websocket.hpp"
#include "boost/asio/ip/tcp.hpp"

#include <cstdlib>
// #include <functional>
// #include <iostream>
// #include <string>
// #include <thread>
// #include <memory>
#include <set>
#include <deque>
// #include <chrono>
#include <cassert>

using namespace std;
// using namespace std::chrono_literals;
// using namespace mpcs_51044_final_project::v1;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

atomic_uint32_t sid = 0;

struct Packet
{
    string textBuffer;
    vector<uint8_t> binaryBuffer;
    bool text = false;
};

using PacketPtr = shared_ptr<Packet>;

class Session : public enable_shared_from_this<Session>
{
public:
    Session(tcp::socket &&socket)
        : m_id(++sid), m_ws(move(socket))
    {
        // set suggested timeout settings for the websocket
        m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

        // set a decorator to change the Server of the handshake
        m_ws.set_option(websocket::stream_base::decorator(
            [](websocket::response_type &res)
            {
                res.set(http::field::server, string(BOOST_BEAST_VERSION_STRING) + "ws-simple-server");
            }));

        cout << "Created session " << m_id << '\n';

        // Create MultiTimer
        timer_p = make_unique<MultiTimer<void()>>(1);
    }

    void run()
    {
        m_ws.async_accept(beast::bind_front_handler(&Session::onAccept, shared_from_this()));
    }

    void onAccept(beast::error_code e)
    {
        if (e)
            return fail(e, "accept");
        doRead();
    }

    void doRead()
    {
        m_ws.async_read(m_readBuf, beast::bind_front_handler(&Session::onRead, shared_from_this()));
    }

    void onRead(beast::error_code e, size_t /*length*/)
    {
        if (e == websocket::error::closed)
            return close();
        if (e)
            return fail(e, "read");

        if (!m_ws.got_text())
        {
            cout << "o_O Got non text. Ignoring.\n";
        }
        else
        {
            auto packet = make_shared<Packet>();
            packet->text = true;
            auto data = reinterpret_cast<const char *>(m_readBuf.cdata().data());
            packet->textBuffer.assign(data, data + m_readBuf.size());
            onReceive(packet);
        }

        m_readBuf.clear();
        doRead(); // read loop
    }

    void onReceive(const PacketPtr &packet)
    {
        cout << "Received: " << packet->textBuffer << '\n';
        // echo
        // send(packet);

        // Set Timer
        if (packet->textBuffer.find("SET") == 0)
        {
            auto cb = bind(&Session::callback, this);
            auto timeout = stoi(packet->textBuffer.substr(4)); // SET:5 -> 5
            timer_p->set(0, chrono::seconds(timeout), cb);
            sendMsg("SET DONE!");

        }
        else if (packet->textBuffer.find("CANCEL") == 0)
        {
            timer_p->cancel(0);
            sendMsg("CANCELLED");
        }
        else if (packet->textBuffer.find("RESET") == 0)
        {
            auto cb = bind(&Session::callback, this);
            auto timeout = stoi(packet->textBuffer.substr(6)); // RESET:5 -> 5
            timer_p->reset(0, chrono::seconds(timeout), cb);
            sendMsg("RESET DONE!");
        }
    }

    void send(const PacketPtr &packet)
    {
        m_writeQueue.emplace_back(packet);
        if (m_writeQueue.size() > 1)
            return; // we're already writing

        doWrite();
    }

    void doWrite()
    {
        assert(!m_writeQueue.empty());

        auto &packet = m_writeQueue.front();
        m_ws.text(packet->text);
        auto handler = beast::bind_front_handler(&Session::onWrite, shared_from_this());
        if (packet->text)
            m_ws.async_write(net::buffer(packet->textBuffer), std::move(handler));
        else
            m_ws.async_write(net::buffer(packet->binaryBuffer), std::move(handler));
    }

    void onWrite(beast::error_code e, std::size_t)
    {
        if (e)
            return fail(e, "write");

        m_writeQueue.pop_front();
        if (m_writeQueue.empty())
            return;

        doWrite();
    }

    void fail(beast::error_code e, const char *source)
    {
        cerr << "Session " << m_id << " error: " << e << " in " << source << '\n';
    }

    void close()
    {
        cout << "Session " << m_id << " closed \n";
    }

    void sendMsg(string msg)
    {
        auto p = make_shared<Packet>();
        p->textBuffer = msg;
        p->text = true;
        send(p);
    }

    void callback()
    {
        sendMsg("CALLBACK EXECUTED"s);
    }

private:
    const uint32_t m_id;
    websocket::stream<tcp::socket> m_ws;

    // io
    beast::flat_buffer m_readBuf, m_writeBuf;

    deque<PacketPtr> m_writeQueue;

    unique_ptr<MultiTimer<void()>> timer_p;
};

class Server
{
public:
    Server(tcp::endpoint endpoint)
        : m_context(1), m_acceptor(m_context, endpoint)
    {
    }

    int run()
    {
        doAccept();
        m_context.run();
        return 0;
    }

    void doAccept()
    {
        m_acceptor.async_accept(beast::bind_front_handler(&Server::onAccept, this));
    }

    void onAccept(beast::error_code error, tcp::socket socket)
    {
        if (error)
        {
            cerr << "Server::onAccept error: " << error << '\n';
            return;
        }

        auto session = make_shared<Session>(move(socket));
        session->run();

        doAccept();
    }

private:
    net::io_context m_context;
    tcp::acceptor m_acceptor;
};

int main()
{
    ///////////////////////////////////////////////////////////////////////////
    // args
    const auto address = boost::asio::ip::tcp::v4(); // net::ip::make_address(argAddr);
    const uint16_t port = 7654;

    ///////////////////////////////////////////////////////////////////////////
    // run
    Server server(tcp::endpoint(address, port));
    return server.run();
}