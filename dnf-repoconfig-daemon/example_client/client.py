#!/usr/bin/env python3

# Copyright (C) 2020 Red Hat, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

import dbus

bus = dbus.SystemBus()
proxy = bus.get_object('org.rpm.dnf.v0.rpm.RepoConf', '/org/rpm/dnf/v0/rpm/RepoConf')
iface = dbus.Interface(proxy, dbus_interface='org.rpm.dnf.v0.rpm.RepoConfSession')

session = iface.open_session("/")

proxy_rconf = bus.get_object('org.rpm.dnf.v0.rpm.RepoConf', session)
iface_rconf = dbus.Interface(proxy_rconf, dbus_interface='org.rpm.dnf.v0.rpm.RepoConf')

for repo in iface_rconf.list([]):
    print(repo['repoid'])

print(iface_rconf.get("fedora"))

#print(iface_rconf.disable(["fedora"]))
#print(iface_rconf.enable(["fedora"]))

print(iface.close_session(session))
