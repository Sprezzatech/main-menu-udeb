#ifndef _NETCFG_H_
#define _NETCFG_H_
#include <sys/types.h>
#include <stdio.h>
#include <cdebconf/debconfclient.h>

#define ETC_DIR 	"/etc"
#define NETWORK_DIR 	"/etc/network"
#define DHCPCD_DIR 	"/etc/dhcpc"
#define INTERFACES_FILE "/etc/network/interfaces"
#define HOSTS_FILE      "/etc/hosts"
#define MAIL_FILE       "/etc/mailname"
#define NETWORKS_FILE   "/etc/networks"
#define RESOLV_FILE     "/etc/resolv.conf"
#define DHCPCD_FILE     "/etc/dhcpc/config"
#define DHCLIENT_DIR	"/var/dhcp"

#define _GNU_SOURCE

extern int netcfg_progress_displayed;

/* network config */
extern char *interface;
extern char *hostname;
extern char *dhcp_hostname;
extern char *domain;
extern u_int32_t ipaddress;
extern u_int32_t nameserver_array[4];
extern u_int32_t network;
extern u_int32_t broadcast;
extern u_int32_t netmask;
extern u_int32_t gateway;
extern u_int32_t pointopoint;

/* common functions */
extern int netcfg_mkdir (char *path);

extern int is_interface_up (char *inter);

extern void get_name (char *name, char *p);

extern void getif_start ();

extern void getif_end ();

extern char *get_ifdsc (struct debconfclient *client, const char *ifp);

extern FILE *file_open (char *path, const char *opentype);

extern void dot2num (u_int32_t * num, char *dot);

extern char *num2dot (u_int32_t num);

extern void netcfg_die (struct debconfclient *client);

extern int netcfg_get_interface(struct debconfclient *client, char **interface, int *num_interfaces);

extern int netcfg_get_hostname(struct debconfclient *client, char **hostname);

extern int netcfg_get_nameservers (struct debconfclient *client, char **nameservers);

extern int netcfg_get_domain(struct debconfclient *client,  char **domain);

extern int netcfg_get_dhcp(struct debconfclient *client);

extern int netcfg_get_static(struct debconfclient *client);

extern int netcfg_activate_dhcp(struct debconfclient *client);

extern int netcfg_activate_static(struct debconfclient *client);

extern int my_debconf_input(struct debconfclient *client, char *priority, char *template, char **result);

extern void netcfg_write_common (const char *prebaseconfig,
				 u_int32_t ipaddress, char *hostname);

void netcfg_nameservers_to_array(char *nameservers, u_int32_t array[]);

#endif /* _NETCFG_H_ */
