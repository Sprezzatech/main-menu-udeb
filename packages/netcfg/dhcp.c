/*
 * DHCP module for netcfg/netcfg-dhcp.
 *
 * Licensed under the terms of the GNU General Public License
 */

#include "netcfg.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <debian-installer.h>
#include <stdio.h>
#include <assert.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>

int dhcp_running = 0, dhcp_exit_status = 1;

/* Signal handler for DHCP client child */
static void dhcp_client_sigchld(int sig __attribute__ ((unused))) 
{
    if (dhcp_running == 1) {
        di_info("in sighandler, PID = %d", dhcp_pid);
	dhcp_running = 0;
	wait(&dhcp_exit_status);
	di_info("exited");
    }
}

static void netcfg_write_dhcp (char* prebaseconfig, char *iface)
{
    FILE *fp;

    if ((fp = file_open(INTERFACES_FILE, "a"))) {
        fprintf(fp,
                "\n# This entry was created during the Debian installation\n");
        if (!iface_is_hotpluggable(iface))
            fprintf(fp, "auto %s\n", iface);
        fprintf(fp, "iface %s inet dhcp\n", iface);
	if (is_wireless_iface(iface))
	{
	  fprintf(fp, "\twireless_mode %s\n",
	      (mode == MANAGED) ? "managed" : "adhoc");
	  fprintf(fp, "\twireless_essid %s\n", essid ? essid : "any");
	  if (wepkey != NULL)
	    fprintf(fp, "\twireless_key %s\n", wepkey);
	}
        fclose(fp);
    }

    if ((fp = file_open(RESOLV_FILE, "a"))) {
      fclose(fp);
    }
    
    di_system_prebaseconfig_append(prebaseconfig, "cp %s %s\n", RESOLV_FILE,
	"/target" RESOLV_FILE);
}

#define DHCP_SECONDS 15

/* 
 * This function will start whichever DHCP client is available (if
 * necessary). That's all. The meat of the DHCP code is really in
 * poll_dhcp_client.
 *
 * Its PID will be left in dhcp_pid.
 * If the client is not running, dhcp_pid will always get set to -1.
 */

int start_dhcp_client (struct debconfclient *client, char* dhostname)
{
  FILE *dc = NULL;
  dhclient_t dhcp_client;

  if (access("/var/lib/dhcp3", F_OK) == 0)
    dhcp_client = DHCLIENT3;
  else if (access("/sbin/dhclient", F_OK) == 0)
    dhcp_client = DHCLIENT;
  else if (access("/sbin/pump", F_OK) == 0)
    dhcp_client = PUMP;
  else if (access("/sbin/udhcpc", F_OK) == 0)
    dhcp_client = UDHCPC;
  else {
    debconf_input(client, "critical", "netcfg/no_dhcp_client");
    debconf_go(client);
    exit(1);
  }

  /* Some clients need this ... */
  ifconfig_up (interface);

  if ((dhcp_pid = fork()) == 0) /* child */
  {
    di_info("in child, PID = %d", getpid());
    
    /* get dhcp lease */
    switch (dhcp_client)
    {
      case PUMP:
	if (dhostname)
	  execlp("pump", "pump", "-i", interface, "-h", dhostname, NULL);
	else
	  execlp("pump", "pump", "-i", interface, NULL);

	break;

      case DHCLIENT:
	/* First, set up dhclient.conf if necessary */

	if (dhostname)
	{
	  if ((dc = file_open(DHCLIENT_CONF, "w")))
	  {
	    fprintf(dc, "send host-name \"%s\";\n", dhostname);
	    fclose(dc);
	  }
	}

	execlp("dhclient", "dhclient", "-e", interface, NULL);
	break;

      case DHCLIENT3:
	/* Different place.. */

	if (dhostname)
	{
	  if ((dc = file_open(DHCLIENT3_CONF, "w")))
	  {
	    fprintf(dc, "send host-name \"%s\";\n", dhostname);
	    fclose(dc);
	  }
	}

	execlp("dhclient", "dhclient", "-1", interface, NULL);
	break;

      case UDHCPC:
	if (dhostname)
	  execlp("udhcpc", "udhcpc", "-i", interface, "-n", "-H", dhostname, NULL);
	else
	  execlp("udhcpc", "udhcpc", "-i", interface, "-n", NULL);

	break;
    }
    di_error("reached end of switch!! dhcp_client = %d", dhcp_client);
    if (errno != 0)
      di_error("exec died with: %s", strerror(errno));

    return 1; /* should NEVER EVER get here */
  }
  else if (dhcp_pid == -1)
  {
    di_error("oh shit, PID is -1");
    return 1;
  }
  else
  {
    di_info("in parent, PID = %d, child PID = %d", getpid(), dhcp_pid);
    dhcp_running = 1;
    signal(SIGCHLD, &dhcp_client_sigchld);
    return 0;
  }
}

/* Poll the started DHCP client for ten seconds, and return 0 if a lease was
 * acquired, 1 otherwise. The client should die once a lease is acquired.
 *
 * It will NOT reap the DHCP client after an unsuccessful poll. 
 */

int poll_dhcp_client (struct debconfclient *client)
{
  time_t start_time, now;
  int ret = 1;

  /* show progress bar */
  debconf_progress_start(client, 0, DHCP_SECONDS, "netcfg/dhcp_progress");
  debconf_progress_info(client, "netcfg/dhcp_progress_note");
  netcfg_progress_displayed = 1;

  now = start_time = time(NULL);

  /* wait 10s for a DHCP lease */
  while (dhcp_running && ((now - start_time) < DHCP_SECONDS)) {
    sleep(1);
    debconf_progress_step(client, 1);
    now = time(NULL);
  }

  /* got a lease? display a success message */
  if (!dhcp_running && (dhcp_exit_status == 0))
  {
    dhcp_pid = -1;
    ret = 0;

    debconf_progress_set(client, DHCP_SECONDS);
    debconf_progress_info(client, "netcfg/dhcp_success_note");
    sleep(2);
  }
  
  /* stop progress bar */
  debconf_progress_stop(client);
  netcfg_progress_displayed = 0;
  
  return ret;
}

int ask_dhcp_retry (struct debconfclient *client)
{
  int ret;
  
  if (is_wireless_iface(interface))
  {
    debconf_metaget(client, "netcfg/internal-wifireconf", "description");
    debconf_subst(client, "netcfg/dhcp_retry", "wifireconf", client->value);
  }
  else /* blank from last time */
    debconf_subst(client, "netcfg/dhcp_retry", "wifireconf", "");

  /* critical, we don't want to enter a loop */
  debconf_input(client, "critical", "netcfg/dhcp_retry");
  ret = debconf_go(client);

  if (ret == 30)
    return GO_BACK;
  
  debconf_get(client, "netcfg/dhcp_retry");

    /* strcmp sucks */
  if (client->value[0] == 'R')
  {
    size_t len = strlen(client->value);
    /* with DHCP hostnam_e_ */
    if (client->value[len - 1] == 'e')
      return 1;
    /* wireless networ_k_ */
    else if (client->value[len - 1] == 'k')
      return 4;
    else
      return 0;
  }
  else if (client->value[0] == 'C')
    return 2; /* manual */
  else if (empty_str(client->value))
    return 5; /* loop back into dhcp_retry */
  else
    return 3; /* no config */
}

/* Here comes another Satan machine. */
int netcfg_activate_dhcp (struct debconfclient *client)
{
  char* dhostname = NULL;
  enum { START, ASK_RETRY, POLL, DHCP_HOSTNAME, HOSTNAME, DOMAIN, STATIC, END } state = START;

  kill_dhcp_client();
  loop_setup();

  for (;;)
  {
    switch (state)
    {
      case START:
        if (start_dhcp_client(client, dhostname))
          netcfg_die(client); /* change later */
        else
          state = POLL;
        break;

      case DHCP_HOSTNAME:
        if (netcfg_get_hostname(client, "netcfg/dhcp_hostname", &dhostname, 0))
          state = ASK_RETRY;
        else
        {
          if (empty_str(dhostname))
          {
            free(dhostname);
            dhostname = NULL;
          }
          kill_dhcp_client();
          state = START;
        }
        break;

      case ASK_RETRY:
        /* ooh, now it's a select */
        switch (ask_dhcp_retry (client))
        {
          case GO_BACK: kill_dhcp_client(); exit(10); /* XXX */
          case 0: state = POLL; break;
          case 1: state = DHCP_HOSTNAME; break;
          case 2: state = STATIC; break;
          case 3: /* no net config at this time :( */
                  kill_dhcp_client();
		  netcfg_write_loopback("40netcfg");
                  return 0;
	  case 4: /* reconfig wifi */
          {
	    /* oh god - a NESTED satan machine */
 	    enum { ABORT, DONE, ESSID, WEP } wifistate = ESSID;

	    for (;;)
	    {
	      switch (wifistate)
	      {
		case ESSID:
		  state = ( netcfg_wireless_set_essid(client, interface) == GO_BACK ) ?
		    ABORT : WEP;
		  break;

		case WEP:
		  state = ( netcfg_wireless_set_wep (client, interface) == GO_BACK ) ?
		    ESSID : DONE;
		  break;

		case ABORT:
		  exit(10);
		  break;

		case DONE: break; /* avoid warning */
	      }

	      if (state == DONE)
		break;
	    }
	  }
        }
        break;

      case POLL:
        if (poll_dhcp_client(client)) /* could not get a lease */
          state = ASK_RETRY;
        else
        {
          char buf[MAXHOSTNAMELEN + 1];

          /* dhcp hostname, ask for one with the dhcp hostname
           * as a seed */
          if (gethostname(buf, sizeof(buf)) == 0 && strcmp(buf, "(none)") != 0)
            debconf_set(client, "netcfg/get_hostname", buf);
          else
            seed_hostname_from_dns(client);

          state = HOSTNAME;
        }
        break;

      case HOSTNAME:
        if (netcfg_get_hostname (client, "netcfg/get_hostname", &hostname, 1))
	{
	  kill_dhcp_client();
          exit(10); /* go back, going back to poll isn't intuitive */
	}
        else
          state = DOMAIN;
        break;

      case DOMAIN:
	if (!have_domain && netcfg_get_domain (client, &domain))
	  state = HOSTNAME;
	else
	  state = END;
	break;

      case STATIC:
	kill_dhcp_client();
        return 15;
        break;
        
      case END:
        /* write configuration */
        netcfg_write_common("40netcfg", ipaddress, hostname, domain);
        netcfg_write_dhcp("40netcfg", interface);
        
        return 0;
    }
  }
} 

int kill_dhcp_client(void)
{
  system("killall.sh"); 
  return 0;
}
