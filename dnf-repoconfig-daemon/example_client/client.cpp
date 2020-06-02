/*
 * Copyright (C) 2020 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    auto connection = sdbus::createSystemBusConnection();
    std::cout << "Client name: " << connection->getUniqueName() << std::endl;
    const char* destination_name = "org.rpm.dnf.v0.rpm.RepoConf";
    const char* object_path = "/org/rpm/dnf/v0/rpm/RepoConf";
    auto repoconf_session_proxy = sdbus::createProxy(*connection, destination_name, object_path);
    const char* session_interface = "org.rpm.dnf.v0.rpm.RepoConfSession";
    const char* repoconf_interface = "org.rpm.dnf.v0.rpm.RepoConf";
    repoconf_session_proxy->finishRegistration();

    sdbus::ObjectPath session_object_path;
    repoconf_session_proxy->callMethod("open_session").onInterface(session_interface).withArguments("/").storeResultsTo(session_object_path);

    auto repoconf_proxy = sdbus::createProxy(*connection, destination_name, session_object_path);
    repoconf_session_proxy->finishRegistration();

    {
        std::vector<std::string> ids = {"fedora", "updates"};
        std::vector<std::map<std::string, sdbus::Variant>> repolist;
        repoconf_proxy->callMethod("list").onInterface(repoconf_interface).withArguments(ids).storeResultsTo(repolist);
        for (auto &repo: repolist) {
            for (auto &item: repo) {
                std::cout << item.first << ": " << item.second.get<std::string>() << std::endl;
            }
            std::cout << "-----------" << std::endl;
        }
    }

    repoconf_session_proxy->callMethod("close_session").onInterface(session_interface).withArguments(session_object_path);

    return 0;
}
