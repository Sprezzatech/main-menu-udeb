/* 
   netcfg.c - Configure the network for the debian-installer
   Author - David Whedon
 

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <debconfclient.h>

#include "util.h"

static char *interface = NULL;
static char *hostname = NULL;
static char *domain = NULL;
static char *dhcp_hostname = NULL;
static int do_dhcp=0;
static u_int32_t ipaddress = 0;
static u_int32_t network = 0;
static u_int32_t broadcast = 0;
static u_int32_t netmask = 0;
static u_int32_t gateway = 0;

static u_int32_t nameservers[4] = { 0 };

static struct debconfclient *client;


static char buf[MAXLINE];

#ifdef DEBUG
#define INTERFACES_FILE "etc/network/interfaces"
#define HOSTS_FILE      "etc/hosts"
#define NETWORKS_FILE   "etc/networks"
#define RESOLV_FILE     "etc/resolv.conf"
#define DHCPCD_FILE     "etc/dhcpc/config"
#else
#define INTERFACES_FILE "/etc/network/interfaces"
#define HOSTS_FILE      "/etc/hosts"
#define NETWORKS_FILE   "/etc/networks"
#define RESOLV_FILE     "/etc/resolv.conf"
#define DHCPCD_FILE     "/etc/dhcpc/config"
#endif


/** 
 * dot2num and num2dot
 * Copyright: Karl Hammar, Asp� Data
*/
char *
dot2num (u_int32_t * num, char *dot)
{
  char *p = dot - 1;
  char *e;
  int ix;
  unsigned long val;

  if (!dot)
    return NULL;
  *num = 0;
  for (ix = 0; ix < 4; ix++)
    {
      *num <<= 8;
      p++;
      val = strtoul (p, &e, 10);
      if (e == p)
	val = 0;
      else if (val > 255)
	return NULL;
      *num += val;
      /*printf("%#8x, %#2x\n", *num, val); */
      if (ix < 3 && *e != '.')
	return NULL;
      p = e;
    }

  return p;
}


static char num2dot_buf[16];

char *
num2dot (u_int32_t num)
{
  int byte[4];
  int ix;
  char *dot = num2dot_buf;

  if (num == 0)
    return NULL;

  for (ix = 3; ix >= 0; ix--)
    {
      byte[ix] = num & 0xff;
      num >>= 8;
    }
  sprintf (dot, "%d.%d.%d.%d", byte[0], byte[1], byte[2], byte[3]);

  return dot;
}


char *
debconf_input (char *priority, char *template)
{
  client->command (client, "input", priority, template, NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", template, NULL);
  return client->value;
}

void
debconf_subst (char *template, char *key, char *string)
{
  if (string)
    client->command (client, "subst", template, key, string, NULL);
  else
    client->command (client, "subst", template, key, "<none>", NULL);
}


void
debconf_unseen (char *template)
{
  client->command (client, "fset", template, "seen", "false", NULL);
}


/*
 * Get all available interfaces from the kernel and ask the user which one
 * he wants to configure
 */
int
get_interface ()
{
  char *ptr = buf;
  char *inter;

  debconf_unseen ("netcfg/choose_interface");

  getif_start ();
  while ((inter = getif (1)) != NULL)
    {
      ptr +=
	snprintf (ptr, sizeof (buf) - strlen (buf), "%s: %s, ", inter,
		  get_ifdsc (inter));
      if (ptr > (buf + sizeof (buf)))
	{
	  fprintf (stderr, "Internal error.\n");
	  exit (1);
	}
    }
  getif_end ();

  if (ptr == buf)
    {
      client->command (client, "input", "high", "netcfg/no_interfaces", NULL);
      client->command (client, "go", NULL);
      return -1;
    }


  debconf_subst ("netcfg/choose_interface", "ifchoices", buf);

  ptr = debconf_input ("critical", "netcfg/choose_interface");

  /* grab just the interface name, not the description too */
  *(strchr (ptr, ':')) = '\0';

  interface = strdup (ptr);
  debconf_subst ("netcfg/confirm_static_cfg", "interface", interface);

  return 0;
}


void
get_static_cfg (void)
{
  char finished = 0;
  char *ptr, *ns;

  do
    {
      get_interface ();

      debconf_unseen ("netcfg/get_hostname");
      debconf_unseen ("netcfg/get_ipaddress");
      debconf_unseen ("netcfg/get_netmask");
      debconf_unseen ("netcfg/get_gateway");
      debconf_unseen ("netcfg/get_nameservers");
      debconf_unseen ("netcfg/confirm_static_cfg");
      if (hostname)
	free (hostname);
      if (domain)
	free (domain);

      hostname = domain = NULL;
      ipaddress = network = broadcast = netmask = gateway =
	nameservers[0] = 0;

      hostname = strdup (debconf_input ("high", "netcfg/get_hostname"));

      debconf_subst ("netcfg/confirm_static_cfg", "hostname", hostname);

      if ((ptr = debconf_input ("high", "netcfg/get_domain")))
	domain = strdup (ptr);

      debconf_subst ("netcfg/confirm_static_cfg", "domain", domain);

      if ((ptr = debconf_input ("high", "netcfg/get_ipaddress")))
	dot2num (&ipaddress, ptr);

      debconf_subst ("netcfg/confirm_static_cfg", "ipaddress",
		     num2dot (ipaddress));

      if ((ptr = debconf_input ("high", "netcfg/get_netmask")))
	dot2num (&netmask, ptr);
      debconf_subst ("netcfg/confirm_static_cfg", "netmask",
		     num2dot (netmask));

      network = ipaddress & netmask;

      debconf_subst ("netcfg/confirm_static_cfg", "network",
		     num2dot (network));


      if ((ptr = debconf_input ("high", "netcfg/get_gateway")))
	dot2num (&gateway, ptr);
      debconf_subst ("netcfg/confirm_static_cfg", "gateway",
		     num2dot (gateway));


      if ((gateway & netmask) != (ipaddress & netmask))
	{
	  client->command (client, "input", "high",
			   "netcfg/gateway_unreachable", NULL);
	  client->command (client, "go", NULL);
	}

      broadcast = (network | ~netmask);
      debconf_subst ("netcfg/confirm_static_cfg", "broadcast",
		     num2dot (broadcast));

      ptr = debconf_input ("high", "netcfg/get_nameservers");

      if (ptr)
	{
	  char *save;
	  save = ptr = strdup (ptr);
	  ns = strtok_r (ptr, " ", &ptr);
	  debconf_subst ("netcfg/confirm_static_cfg", "primary_DNS", ns);
	  dot2num (&nameservers[0], ns);

	  ns = strtok_r (NULL, " ", &ptr);
	  debconf_subst ("netcfg/confirm_static_cfg", "secondary_DNS", ns);
	  dot2num (&nameservers[1], ns);

	  ns = strtok_r (NULL, " ", &ptr);
	  debconf_subst ("netcfg/confirm_static_cfg", "tertiary_DNS", ns);
	  dot2num (&nameservers[2], ns);

	  free (save);
	}
      else
	{
	  debconf_subst ("netcfg/confirm_static_cfg", "primary_DNS", NULL);
	  debconf_subst ("netcfg/confirm_static_cfg", "secondary_DNS", NULL);
	  debconf_subst ("netcfg/confirm_static_cfg", "tertiary_DNS", NULL);
	}


      ptr = debconf_input ("high", "netcfg/confirm_static_cfg");

      if (strstr (ptr, "true"))
	finished = 1;


    }
  while (!finished);

}

FILE *
file_open (char *path)
{
  FILE *fp;
  if ((fp = fopen (path, "w")))
    return fp;
  else
    {
      perror ("fopen");
      return NULL;
    }

}



void
write_static_cfg (void)
{
  FILE *fp;
  int i;

  if ((fp = file_open (HOSTS_FILE)))
    {
      fprintf (fp, "127.0.0.1\tlocalhost\n");
      if (domain)
	{
	  fprintf (fp, "%s\t%s.%s\t%s\n", num2dot (ipaddress),
		   hostname, domain, hostname);
	}
      else
	{
	  fprintf (fp, "%s\t%s\n", num2dot (ipaddress), hostname);
	}

      fclose (fp);
    }

  if ((fp = file_open (NETWORKS_FILE)))
    {
      fprintf (fp, "localnet %s\n", num2dot (network));
      fclose (fp);
    }

  if ((fp = file_open (RESOLV_FILE)))
    {
      i = 0;
      if (domain)
	fprintf (fp, "search %s\n", domain);
      while (nameservers[i])
	{
	  fprintf (fp, "nameserver %s\n", num2dot (nameservers[i++]));
	}
      fclose (fp);
    }

  if ((fp = file_open (INTERFACES_FILE)))
    {
      fprintf (fp,
	       "\n# This entry was created during the Debian installation\n");
      fprintf (fp, "# (network, broadcast and gateway are optional)\n");
      fprintf (fp, "iface %s inet static\n", interface);
      fprintf (fp, "\taddress %s\n", num2dot (ipaddress));
      fprintf (fp, "\tnetmask %s\n", num2dot (netmask));
      fprintf (fp, "\tnetwork %s\n", num2dot (network));
      fprintf (fp, "\tbroadcast %s\n", num2dot (broadcast));
      fprintf (fp, "\tgateway %s\n", num2dot (gateway));
      fclose (fp);
    }

}



void
write_dhcp_cfg(void){

  FILE *fp;
  if ((fp = file_open (INTERFACES_FILE)))
    {
      fprintf (fp, "\n# This entry was created during the Debian installation\n");
      fprintf (fp, "iface %s inet dhcp\n", interface);
    }
  if ((fp = file_open (DHCPCD_FILE)))
  {
      fprintf (fp, "\n# dhcpcd configuration: created during the Debian installation\n");
      fprintf (fp, "IFACE=%s", interface);
      if (dhcp_hostname)
	  fprintf (fp, "OPTIONS='-h %s'", dhcp_hostname);
  }
}


int
activate_net ()
{
    char *ptr;
    int rv;
  system ("/sbin/ifconfig lo 127.0.0.1");

  if (do_dhcp) {
      ptr=buf;
      ptr += snprintf (buf, sizeof (buf), "/sbin/dhcpcd-2.2.x");
      if (dhcp_hostname)
    	  snprintf(ptr, sizeof(buf)-(ptr-buf), " -h %s", dhcp_hostname);

  }
  else {
      snprintf (buf, sizeof (buf),
  	      "/sbin/ifconfig %s %s netmask %s broadcast %s", interface,
  	      num2dot (ipaddress), num2dot (netmask), num2dot (broadcast));
  }
  
  rv = system (buf);
  fprintf(stderr, "rv = %d\n", rv);
  if (rv != 0) {
      client->command (client, "input", "critical", "netcfg/error_cfg", NULL);
      client->command (client, "go", NULL);
  }
  return 0;
}



int
main (int argc, char *argv[])
{

    char *ptr;
  client = debconfclient_new ();

  client->command (client, "title", "Network Configuration", NULL);

  ptr = debconf_input ("critical", "netcfg/dhcp_option");

  if (strstr (ptr, "true"))
  {
      do_dhcp=1;
      dhcp_hostname = debconf_input ("high", "netcfg/dhcp_hostname");
      write_dhcp_cfg();
  }
  else 
  {
      get_static_cfg ();
      write_static_cfg ();
  }

  activate_net();

  return 0;
}
