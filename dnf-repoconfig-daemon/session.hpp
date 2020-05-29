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

#ifndef DNF_REPOCONFIG_DAEMON_SESSION_HPP
#define DNF_REPOCONFIG_DAEMON_SESSION_HPP

#include "repoconf.hpp"

#include <sdbus-c++/sdbus-c++.h>

#include <mutex>
#include <vector>
#include <string>

class CallBackTimer;

class Session {
public:
    Session(sdbus::IConnection &connection);
    ~Session();
    void open_session(sdbus::MethodCall call);
    void close_session(sdbus::MethodCall call);
    void drop_stale_sessions();

private:
    std::map<std::string, std::unique_ptr<RepoConf>> sessions;
    std::mutex sessions_mutex;
    sdbus::IConnection &connection;
    std::unique_ptr<sdbus::IObject> dbus_object;
    void dbus_register_methods();
    std::string gen_session_id();
    std::unique_ptr<CallBackTimer> watchdog;
};

#endif
