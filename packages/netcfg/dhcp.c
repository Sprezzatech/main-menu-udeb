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
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <time.h>
#include <netdb.h>


#define DHCP_SECONDS 15


static int dhcp_exit_status = 1;
static pid_t dhcp_pid = -1;


/*
 * Add DHCP-related lines to /etc/network/interfaces
 */
static void netcfg_write_dhcp (char *iface, char *dhostname)
{
    FILE *fp;

    if ((fp = file_open(INTERFACES_FILE, "a"))) {
        fprintf(fp, "\n# The primary network interface\n");
        if (!iface_is_hotpluggable(iface))
            fprintf(fp, "auto %s\n", iface);
        fprintf(fp, "iface %s inet dhcp\n", iface);
        if (dhostname)
        {
          fprintf(fp, "\thostname %s\n", dhostname);
        }
        if (is_wireless_iface(iface))
        {
          fprintf(fp, "\t# wireless-* options are implemented by the wireless-tools package\n");
          fprintf(fp, "\twireless-mode %s\n",
              (mode == MANAGED) ? "managed" : "adhoc");
          fprintf(fp, "\twireless-essid %s\n", essid ? essid : "any");
          if (wepkey != NULL)
            fprintf(fp, "\twireless-key %s\n", wepkey);
        }
        fclose(fp);
    }

#if 0
    if ((fp = file_open(RESOLV_FILE, "a"))) {
      fclose(fp);
    }
#endif
}


/*
 * Signal handler for DHCP client child
 */
static void dhcp_client_sigchld(int sig __attribute__ ((unused))) 
{
  if (dhcp_pid <= 0)
    return;
  /*
   * I hope it's OK to call waitpid() from the SIGCHLD signal handler
   */
  waitpid(dhcp_pid,&dhcp_exit_status,0);
  dhcp_pid = -1;
}


/* 
 * This function will start whichever DHCP client is available
 * using the provided DHCP hostname, if supplied
 *
 * The client's PID is stored in dhcp_pid.  Once the client dies
 * dhcp_pid should be set to -1.
 */
int start_dhcp_client (struct debconfclient *client, char* dhostname)
{
  FILE *dc = NULL;
  enum { DHCLIENT, DHCLIENT3, PUMP } dhcp_client;

  if (access("/var/lib/dhcp3", F_OK) == 0)
    dhcp_client = DHCLIENT3;
  else if (access("/sbin/dhclient", F_OK) == 0)
    dhcp_client = DHCLIENT;
  else if (access("/sbin/pump", F_OK) == 0)
    dhcp_client = PUMP;
  else {
    debconf_input(client, "critical", "netcfg/no_dhcp_client");
    debconf_go(client);
    exit(1);
  }

  if ((dhcp_pid = fork()) == 0) /* child */
  {
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
    }
    if (errno != 0)
      di_error("Could not exec dhcp client: %s", strerror(errno));

    return 1; /* should NEVER EVER get here */
  }
  else if (dhcp_pid == -1)
    return 1;
  else
  {
    /* dhcp_pid contains the child's PID */
    signal(SIGCHLD, &dhcp_client_sigchld);
    return 0;
  }
}


static int kill_dhcp_client(void)
{
  system("killall.sh"); 
  return 0;
}


/*
 * Poll the started DHCP client for DHCP_SECONDS seconds
 * and return 0 if a lease is known to have been acquired,
 * 1 otherwise.
 *
 * The client should be run such that it dies once a lease is acquired.
 *
 * This function will NOT kill the DHCP client after an unsuccessful poll. 
 */
int poll_dhcp_client (struct debconfclient *client)
{
  int seconds_slept = 0;
  int ret = 1;

  /* show progress bar */
  debconf_progress_start(client, 0, DHCP_SECONDS, "netcfg/dhcp_progress");
  debconf_progress_info(client, "netcfg/dhcp_progress_note");
  netcfg_progress_displayed = 1;

  /* wait between 2 and DHCP_SECONDS seconds for a DHCP lease */
  while (
      ((dhcp_pid > 0) || (seconds_slept < 2))
      && (seconds_slept < DHCP_SECONDS)
  ) {
    sleep(1);
    seconds_slept++; /* Not exact but close enough */
    debconf_progress_step(client, 1);
  }

  /*
   * Either the client exited or time ran out, in which case
   * we leave the DHCP client running
   */

  /* got a lease? display a success message */
  if (!(dhcp_pid > 0) && (dhcp_exit_status == 0))
  {
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


#define REPLY_RETRY_AUTOCONFIG       0
#define REPLY_RETRY_WITH_HOSTNAME    1
#define REPLY_CONFIGURE_MANUALLY     2
#define REPLY_DONT_CONFIGURE         3
#define REPLY_RECONFIGURE_WIFI       4
#define REPLY_LOOP_BACK              5

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
  if (client->value[0] == 'R') /* _R_etry ... or _R_econfigure ... */
  {
    size_t len = strlen(client->value);
    if (client->value[len - 1] == 'e') /* ... with DHCP hostnam_e_ */
      return REPLY_RETRY_WITH_HOSTNAME;
    else if (client->value[len - 1] == 'k') /* ... wireless networ_k_ */
      return REPLY_RECONFIGURE_WIFI;
    else
      return REPLY_RETRY_AUTOCONFIG;
  }
  else if (client->value[0] == 'C') /* _C_onfigure ... */
    return REPLY_CONFIGURE_MANUALLY;
  else if (empty_str(client->value))
    return REPLY_LOOP_BACK;
  else
    return REPLY_DONT_CONFIGURE;
}


/* Here comes another Satan machine. */
int netcfg_activate_dhcp (struct debconfclient *client)
{
  char* dhostname = NULL;
  enum { START, POLL, ASK_RETRY, DHCP_HOST_NAME, HOSTNAME, DOMAIN, HOSTNAME_SANS_NETWORK, DOMAIN_SANS_NETWORK } state = START;

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

      case POLL:
        if (poll_dhcp_client(client))
        {
          /* could not get a lease */
          state = ASK_RETRY;
        }
        else
        {
          /* got a lease */
          /* That means that the DHCP client is no longer running */

          /*
           * Set defaults for domain name and hostname
           */

          char buf[MAXHOSTNAMELEN + 1] = { 0 };
          char *ptr = NULL;
          FILE *d = NULL;

          have_domain = 0;
          
          /*
           * Default to the domain name returned via DHCP, if any
           */
          if ((d = fopen(DOMAIN_FILE, "r")) != NULL)
          {
            char domain[_UTSNAME_LENGTH + 1] = { 0 };
            fgets(domain, _UTSNAME_LENGTH, d);
            fclose(d);
            unlink(DOMAIN_FILE);

            if (!empty_str(domain))
            {
              debconf_set(client, "netcfg/get_domain", domain);
              have_domain = 1;
            }
          }

          /*
           * Default to the hostname returned via DHCP, if any,
           * otherwise to the requested DHCP hostname
           * otherwise to the hostname found in DNS for the IP address of the interface
           */
          if (
               gethostname(buf, sizeof(buf)) == 0
               && !empty_str(buf)
               && strcmp(buf, "(none)")
          ) {
            di_info("DHCP hostname: \"%s\"", buf);
            debconf_set(client, "netcfg/get_hostname", buf);
          }
          else if (dhostname)
          {
            debconf_set(client, "netcfg/get_hostname", dhostname);
          }
          else
          {
            struct ifreq ifr;
            struct in_addr d_ipaddr = { 0 };

            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name, interface, IFNAMSIZ);
            if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0)
            {
              d_ipaddr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
              seed_hostname_from_dns(client, &d_ipaddr);
            }
            else
              di_error("ioctl failed (%s)", strerror(errno));
          }

          /*
           * Default to the domain name that is the domain part
           * of the hostname, if any
           */
          if (have_domain == 0 && (ptr = strchr(buf, '.')) != NULL)
          {
            debconf_set(client, "netcfg/get_domain", ptr + 1);
            have_domain = 1;
          }

          state = HOSTNAME;
        }
        break;

      case ASK_RETRY:
        /* DHCP client may still be running */
        switch (ask_dhcp_retry (client))
        {
          case GO_BACK:
            kill_dhcp_client();
            exit(10); /* XXX */
          case REPLY_RETRY_WITH_HOSTNAME:
            kill_dhcp_client();
            state = DHCP_HOST_NAME;
            break;
          case REPLY_CONFIGURE_MANUALLY:
            kill_dhcp_client();
            return 15;
            break;
          case REPLY_DONT_CONFIGURE:
            kill_dhcp_client();
            netcfg_write_loopback();
            state = HOSTNAME_SANS_NETWORK;
            break;
          case REPLY_RETRY_AUTOCONFIG:
            if (dhcp_pid > 0)
              state = POLL;
            else
              state = START;
            break;
          case REPLY_RECONFIGURE_WIFI:
            {
              /* oh god - a NESTED satan machine */
              enum { ABORT, DONE, ESSID, WEP } wifistate = ESSID;
              for (;;)
              {
                switch (wifistate)
                {
                  case ESSID:
                    wifistate = ( netcfg_wireless_set_essid(client, interface, "high") == GO_BACK ) ?
                      ABORT : WEP;
                    break;
                  case WEP:
                    wifistate = ( netcfg_wireless_set_wep (client, interface) == GO_BACK ) ?
                      ESSID : DONE;
                    break;
                  case ABORT:
                    state = ASK_RETRY;
                    break;
                  case DONE:
                    if (dhcp_pid > 0)
                      state = POLL;
		    else
                      state = START;
                    break;
                }
                if (wifistate == DONE || wifistate == ABORT)
                  break;
              }
            }
	    break;
        }
        break;

      case DHCP_HOST_NAME:
        if (netcfg_get_hostname(client, "netcfg/dhcp_hostname", &dhostname, 0))
          state = ASK_RETRY;
        else
        {
          if (empty_str(dhostname))
          {
            free(dhostname);
            dhostname = NULL;
          }
          state = START;
        }
        break;

      case HOSTNAME:
        if (netcfg_get_hostname (client, "netcfg/get_hostname", &hostname, 1))
        {
          /*
           * Going back to POLL wouldn't make much sense.
           * However, it does make sense to go to the retry
           * screen where the user can elect to retry DHCP with
           * a requested DHCP hostname, etc.
           */
          state = ASK_RETRY;
        }
        else
        {
#if 0
          /*
	   * If we haven't already created the DHCLIENT_CONF file
	   * (by calling start_dhcp_client() with dhostname set)
	   * then set up that file now with hostname as the
	   * DHCP hostname to request.  See #236533.
	   */
	  /*
	   * I don't like this because it changes the DHCLIENT_CONF
	   * file from what it was when dhclient successfully obtained
	   * a lease.  Furthermore, the file gets written even if the
	   * client is pump.  --jdthood
	   */
          if (!dhostname)
          {
            FILE* dc = NULL;
            if ((dc = file_open(DHCLIENT_CONF, "w")))
            {
              fprintf(dc, "send host-name \"%s\";\n", hostname);
              fclose(dc);
            }
            /* prebaseconfig will take care of copying it in. */
          }
#endif
      
          state = DOMAIN;
        }
        break;

      case DOMAIN:
        if (!have_domain && netcfg_get_domain (client, &domain))
          state = HOSTNAME;
        else
        {
          netcfg_write_common(ipaddress, hostname, domain);
          netcfg_write_dhcp(interface, dhostname);
          return 0;
        }
        break;

      case HOSTNAME_SANS_NETWORK:
        if (netcfg_get_hostname (client, "netcfg/get_hostname", &hostname, 1))
          state = ASK_RETRY;
        else
          state = DOMAIN_SANS_NETWORK;
        break;

      case DOMAIN_SANS_NETWORK:
        if (!have_domain && netcfg_get_domain (client, &domain))
          state = HOSTNAME_SANS_NETWORK;
        else
        {
          netcfg_write_common(ipaddress, hostname, domain);
          exit(0);
        }
        break;
    }
  }
} 

