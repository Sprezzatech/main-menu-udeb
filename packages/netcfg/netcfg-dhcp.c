/* 
   netcfg-dhcp.c - Configure a network via dhcp for the debian-installer

   Copyright (C) 2000-2002  David Kimdon <dwhedon@debian.org>
   Copyright (C) 2003  Matt Kraai
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <cdebconf/debconfclient.h>
#include <debian-installer.h>
#include "netcfg.h"

int main(int argc, char *argv[])
{
    int num_interfaces;
    static struct debconfclient *client;
    static int requested_wireless_tools = 0;

    enum { BACKUP, GET_INTERFACE, WCONFIG, WCONFIG_WEP, WCONFIG_ESSID, QUIT } state = GET_INTERFACE;

    /* initialize libd-i */
    di_system_init("netcfg-dhcp");

    parse_args(argc, argv);
    open_sockets();

    /* initialize debconf */
    client = debconfclient_new();
    debconf_capb(client,"backup");

    for ( ;; ) {
	switch(state) {
	case BACKUP:
	    return 10;
	case GET_INTERFACE:
	    if (netcfg_get_interface(client, &interface, &num_interfaces))
	      state = BACKUP;
	    else
	    {
	      if (is_wireless_iface(interface))
		state = WCONFIG;
	      else
		state = QUIT;
	    }
	    break;
	case WCONFIG:
            if (requested_wireless_tools == 0)
            {
              requested_wireless_tools = 1;
              di_exec_shell("apt-install wireless-tools");
            }
	    state = WCONFIG_ESSID;
	    break;

	case WCONFIG_ESSID:
	    if (netcfg_wireless_set_essid (client, interface, NULL))
	      state = BACKUP;
	    else
	      state = WCONFIG_WEP;
	    break;

	case WCONFIG_WEP:
            if (netcfg_wireless_set_wep (client, interface))
              state = WCONFIG_ESSID;
	    else
	      state = QUIT;  
	    break;

	case QUIT:
	    switch (netcfg_activate_dhcp(client)) {
            case 0:
		return 0;
            case 10:
	        state = BACKUP;
	        break;
            default:
	        return 1;
	    }
	}
    }

    return 0;
}
