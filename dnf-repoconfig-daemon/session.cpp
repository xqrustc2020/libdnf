/*
 * Copyright (C) 2020 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "repoconf.hpp"
#include "session.hpp"
#include "../libdnf/log.hpp"
#include "../libdnf/utils/tinyformat/tinyformat.hpp"

#include <sdbus-c++/sdbus-c++.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>

class CallBackTimer
{
public:
    CallBackTimer() :_execute(false) {}

    ~CallBackTimer() {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };
    }

    void stop()
    {
        _execute.store(false, std::memory_order_release);
        if( _thd.joinable() )
            _thd.join();
    }

    void start(int interval, std::function<void(void)> func)
    {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };
        _execute.store(true, std::memory_order_release);
        _thd = std::thread([this, interval, func]()
        {
            while (_execute.load(std::memory_order_acquire)) {
                func();
                std::this_thread::sleep_for(std::chrono::seconds(interval));
            }
        });
    }

    bool is_running() const noexcept {
        return ( _execute.load(std::memory_order_acquire) &&
                 _thd.joinable() );
    }

private:
    std::atomic<bool> _execute;
    std::thread _thd;
};




Session::Session(sdbus::IConnection &connection) : connection(connection), watchdog(new CallBackTimer)
{
    dbus_register_methods();
    // start the watch-dog
    watchdog->start(10, std::bind(&Session::drop_stale_sessions, this));

}

Session::~Session() {
    dbus_object->unregister();
}

void Session::dbus_register_methods()
{
    const std::string objectPath = "/org/rpm/dnf/v1/rpm/RepoConf";
    const std::string interfaceName = "org.rpm.dnf.v1.rpm.RepoConfSession";

    dbus_object = sdbus::createObject(connection, objectPath);
    dbus_object->registerMethod(interfaceName, "open_session", "s", "s", [this](sdbus::MethodCall call) -> void {this->open_session(call);});
    dbus_object->registerMethod(interfaceName, "close_session", "s", "b", [this](sdbus::MethodCall call) -> void {this->close_session(call);});
    dbus_object->finishRegistration();
}


void Session::drop_stale_sessions()
{
    std::lock_guard<std::mutex> lg(sessions_mutex);
    auto it = sessions.begin();
    while (it != sessions.end()) {
        if (it->second->age() > 600) {
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}


std::string Session::gen_session_id()
{
    static std::random_device rd;
    static std::uniform_int_distribution<> dist(0, 15);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; i++) {
        ss << dist(rd);
    }
    return ss.str();
}


void Session::open_session(sdbus::MethodCall call)
{
    std::string install_root;
    call >> install_root;

    // generate UUID-like session id
    const std::string sessionid = gen_session_id();
    // store newly created session
    {
        std::lock_guard<std::mutex> lg(sessions_mutex);
        sessions.emplace(sessionid, std::make_unique<RepoConf>(connection, install_root, sessionid));
    }

    auto reply = call.createReply();
    reply << sessionid;
    reply.send();
}


void Session::close_session(sdbus::MethodCall call)
{
    std::string session_id;
    call >> session_id;

    // if the session with given id exists, delete it
    bool retval = false;
    {
        std::lock_guard<std::mutex> lg(sessions_mutex);
        auto session_it = sessions.find(session_id);
        if (session_it != sessions.end()) {
            sessions.erase(session_it);
            retval = true;
        }
    }

    auto reply = call.createReply();
    reply << retval;
    reply.send();
}
