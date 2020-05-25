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

#include <iostream>
#include <random>
#include <sdbus-c++/sdbus-c++.h>
#include <sstream>
#include <string>

Session::Session(sdbus::IConnection &connection) : connection(connection)
{
    dbus_register_methods();
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
    sessions.emplace(sessionid, std::make_unique<RepoConf>(connection, install_root, sessionid));

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
    auto session_it = sessions.find(session_id);
    if (session_it != sessions.end()) {
        sessions.erase(session_it);
        retval = true;
    }

    auto reply = call.createReply();
    reply << retval;
    reply.send();
}
