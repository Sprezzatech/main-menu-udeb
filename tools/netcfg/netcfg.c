/* 
   netcfg.c - Configure the network for the debian-installer
   Author - David Whedon
 

*/
#include <ctype.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <debconfclient.h>
#include "utils.h"
#include "netcfg.h"


static char *
debconf_input (struct debconfclient *client, char *priority, char *template)
{
  client->command (client, "fset", template, "seen", "false", NULL);
  client->command (client, "input", priority, template, NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", template, NULL);
  return client->value;
}


int
netcfg_mkdir (char *path)
{
  if (check_dir (path) == -1)
    if (!mkdir (path, 0700))
      {
	perror ("mkdir");
	return -1;
      }
  return 0;
}


int
is_interface_up (char *inter)
{
  struct ifreq ifr;
  int sfd = -1, ret = -1;

  if ((sfd = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
    goto int_up_done;

  strncpy (ifr.ifr_name, inter, sizeof (ifr.ifr_name));

  if (ioctl (sfd, SIOCGIFFLAGS, &ifr) < 0)
    goto int_up_done;

  ret = (ifr.ifr_flags & IFF_UP) ? 1 : 0;

int_up_done:
  if (sfd != -1)
    close (sfd);
  return ret;
}


void
get_name (char *name, char *p)
{
  while (isspace (*p))
    p++;
  while (*p)
    {
      if (isspace (*p))
	break;
      if (*p == ':')
	{			/* could be an alias */
	  char *dot = p, *dotname = name;
	  *name++ = *p++;
	  while (isdigit (*p))
	    *name++ = *p++;
	  if (*p != ':')
	    {			/* it wasn't, backup */
	      p = dot;
	      name = dotname;
	    }
	  if (*p == '\0')
	    return;
	  p++;
	  break;
	}
      *name++ = *p++;
    }
  *name++ = '\0';
  return;
}



static FILE *ifs = NULL;
static char ibuf[512];

void
getif_start ()
{
  if (ifs != NULL)
    {
      fclose (ifs);
      ifs = NULL;
    }
  if ((ifs = fopen ("/proc/net/dev", "r")) != NULL)
    {
      fgets (ibuf, sizeof (ibuf), ifs);	/* eat header */
      fgets (ibuf, sizeof (ibuf), ifs);	/* ditto */
    }
  return;
}


char *
getif (int all)
{
  char rbuf[512];
  if (ifs == NULL)
    return NULL;

  if (fgets (rbuf, sizeof (rbuf), ifs) != NULL)
    {
      get_name (ibuf, rbuf);
      if (!strcmp (ibuf, "lo"))	/* ignore the loopback */
	return getif (all);	/* seriously doubt there is an infinite number of lo devices */
      if (all || is_interface_up (ibuf) == 1)
	return ibuf;
    }
  return NULL;
}


void
getif_end ()
{
  if (ifs != NULL)
    {
      fclose (ifs);
      ifs = NULL;
    }
  return;
}


char *
get_ifdsc (const char *ifp)
{
  int i;
  struct if_alist_struct
  {
    char *interface;
    char *description;
  }
  interface_alist[] =
  {
    {
    "eth", "Ethernet or Fast Ethernet"}
    ,
    {
    "pcmcia", "PC-Card (PCMCIA) Ethernet or Token Ring"}
    ,
    {
    "tr", "Token Ring"}
    ,
    {
    "arc", "Arcnet"}
    ,
    {
    "slip", "Serial-line IP"}
    ,
    {
    "plip", "Parallel-line IP"}
    ,
    {
    NULL, NULL}
  };

  for (i = 0; interface_alist[i].interface != NULL; i++)
    if (!strncmp (ifp, interface_alist[i].interface,
		  strlen (interface_alist[i].interface)))
      return interface_alist[i].description;
  return "unknown interface";
}


FILE *
file_open (char *path)
{
  FILE *fp;
  if ((fp = fopen (path, "w")))
    return fp;
  else
    {
      fprintf (stderr, "%s\n", path);
      perror ("fopen");
      return NULL;
    }
}


void
dot2num (u_int32_t * num, char *dot)
{
  char *p = dot - 1;
  char *e;
  int ix;
  unsigned long val;

  if (!dot)
    goto error;

  *num = 0;
  for (ix = 0; ix < 4; ix++)
    {
      *num <<= 8;
      p++;
      val = strtoul (p, &e, 10);
      if (e == p)
	val = 0;
      else if (val > 255)
	goto error;
      *num += val;
      /*printf("%#8x, %#2x\n", *num, val); */
      if (ix < 3 && *e != '.')
	goto error;
      p = e;
    }

  return;

error:
  *num = 0;
}


static char num2dot_buf[16];

char *
num2dot (u_int32_t num)
{
  int byte[4];
  int ix;
  char *dot = num2dot_buf;

  for (ix = 3; ix >= 0; ix--)
    {
      byte[ix] = num & 0xff;
      num >>= 8;
    }
  sprintf (dot, "%d.%d.%d.%d", byte[0], byte[1], byte[2], byte[3]);

  return dot;
}



void
netcfg_die (struct debconfclient *client)
{
  client->command (client, "input", "high", "netcfg/error", NULL);
  client->command (client, "go", NULL);
  exit (1);
}


static void
netcfg_get_interface (struct debconfclient *client, char **interface)
{
  char *inter;
  int len;
  int newchars;
  char *ptr;
  int num_interfaces = 0;

  if (*interface)
    {
      free (*interface);
      *interface = NULL;
    }

  if (!(ptr = malloc (128)))
    netcfg_die (client);
  len = 128;
  *ptr = '\0';

  getif_start ();
  while ((inter = getif (1)) != NULL)
    {
      newchars = strlen (inter) + strlen (get_ifdsc (inter)) + 5;
      if (len < (strlen (ptr) + newchars))
	{
	  if (!(ptr = realloc (ptr, len + newchars + 128)))
	    goto error;
	  len += newchars + 128;
	}

      snprintf (ptr + strlen (ptr), len - strlen (ptr), "%s: %s, ", inter,
		get_ifdsc (inter));
      num_interfaces++;
    }
  getif_end ();

  if (num_interfaces == 0)
    {
      client->command (client, "input", "high", "netcfg/no_interfaces", NULL);
      client->command (client, "go", NULL);
      free (ptr);
      exit (1);
    }
  else if (num_interfaces > 1)
    {
      client->command (client, "subst", "netcfg/choose_interface",
		       "ifchoices", ptr, NULL);
      free (ptr);
      inter = debconf_input (client, "high", "netcfg/choose_interface");

      if (!inter)
	netcfg_die (client);
    }
  else if (num_interfaces == 1)
    inter = ptr;

  /* grab just the interface name, not the description too */
  *interface = inter;
  ptr = strchr (inter, ':');
  if (ptr == NULL)
    goto error;
  *ptr = '\0';

  *interface = strdup (*interface);

  return;

error:
  if (ptr)
    free (ptr);

  netcfg_die (client);
}


void
netcfg_get_common (struct debconfclient *client, char **interface,
		   char **hostname, char **domain, char **nameservers)
{
  char *ptr;

  netcfg_get_interface (client, interface);

  if (*hostname)
    {
      free (*hostname);
      *hostname = NULL;
    }
  *hostname =
    strdup (debconf_input (client, "medium", "netcfg/get_hostname"));

  if (*domain)
    {
      free (*domain);
    }
  *domain = NULL;
  if ((ptr = debconf_input (client, "medium", "netcfg/get_domain")))
    *domain = strdup (ptr);


  if (*nameservers)
    {
      free (*nameservers);
    }
  *nameservers = NULL;
  if (ptr = debconf_input (client, "medium", "netcfg/get_nameservers"))
    *nameservers = strdup (ptr);

}

void
netcfg_nameservers_to_array (char *nameservers, u_int32_t array[])
{

  char *save, *ptr, *ns;

  if (nameservers)
    {
      save = ptr = strdup (ptr);
      ns = strtok_r (ptr, " ", &ptr);
      dot2num (&array[0], ns);

      ns = strtok_r (NULL, " ", &ptr);
      dot2num (&array[1], ns);

      ns = strtok_r (NULL, " ", &ptr);
      dot2num (&array[2], ns);

      array[3] = 0;
      free (save);
    }
  else
    array[0] = 0;

}



void
netcfg_write_common (u_int32_t ipaddress, char *domain, char *hostname,
		     u_int32_t nameservers[])
{
  FILE *fp;


  netcfg_mkdir (ETC_DIR);
  netcfg_mkdir (NETWORK_DIR);


  if ((fp = file_open (HOSTS_FILE)))
    {
      fprintf (fp, "127.0.0.1\tlocalhost\n");
      if (ipaddress)
	{
	  if (domain)
	    fprintf (fp, "%s\t%s.%s\t%s\n", num2dot (ipaddress),
		     hostname, domain, hostname);
	  else
	    fprintf (fp, "%s\t%s\n", num2dot (ipaddress), hostname);
	}

      fclose (fp);
    }

  if ((fp = file_open (RESOLV_FILE)))
    {
      int i = 0;
      if (domain)
	fprintf (fp, "search %s\n", domain);

      while (nameservers[i])
	fprintf (fp, "nameserver %s\n", num2dot (nameservers[i++]));

      fclose (fp);
    }
}
