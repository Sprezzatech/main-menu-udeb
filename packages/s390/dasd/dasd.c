#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cdebconf/debconfclient.h>
#include <debian-installer.h>

const char *const file_devices = "/proc/dasd/devices";
const char *const file_subchannels = "/proc/subchannels";
//const char *const file_devices = "/dev/null";
//const char *const file_subchannels = "subchannels";

static struct debconfclient *client;

static struct dasd
{
	unsigned int device;
	unsigned int devtype;
	enum { UNKNOWN, NEW, UNFORMATED, FORMATED, READY } state;
}
*dasds, *dasd_current;

static unsigned int dasds_items;

enum state_wanted { WANT_BACKUP, WANT_NEXT, WANT_QUIT, WANT_ERROR };

int my_debconf_input(char *priority, char *template, char **ptr)
{
	int ret;
	debconf_fset (client, template, "seen", "false");
	debconf_input (client, priority, template);
	ret = debconf_go (client);
	debconf_get (client, template);
	*ptr = client->value;
	return ret;
}

static bool update_state (void)
{
	char buf[256];
	FILE *f = fopen(file_devices, "r");
	unsigned int i;

	if (!f)
		return false;

	while (fgets (buf, sizeof (buf), f))
	{
		unsigned int device;
		sscanf (buf, "%4x", &device);
		for (i = 0; i < dasds_items; i++)
			if (device == dasds[i].device)
			{
				if (!strncmp (buf + 40, "active", 6))
				{
					if (!strncmp (buf + 47, "n/f", 3))
						dasds[i].state = UNFORMATED;
					else
						dasds[i].state = FORMATED;
				}
				else if (!strncmp (buf + 40, "ready", 5))
					dasds[i].state = READY;
				else
					dasds[i].state = UNKNOWN;
				break;
			}
	}

	return true;
}

static enum state_wanted get_channel (void)
{
	FILE *f;
	char buf[256], buf1[100], *ptr;
	unsigned int i;
	int ret;

	dasds = di_new (struct dasd, 5);
	dasd_current = NULL;
	dasds_items = 0;

	f = fopen(file_subchannels, "r");
	
	if (!f)
		return WANT_ERROR;

	fgets (buf, sizeof (buf), f);
	fgets (buf, sizeof (buf), f);

	while (fgets (buf, sizeof (buf), f))
	{
		if (sscanf (buf, "%4x %*4x %4x/%*2x %*4x/%*2x %s ",
					&dasds[dasds_items].device,
					&dasds[dasds_items].devtype,
					buf1) == 3)
			if(dasds[dasds_items].devtype == 0x3390 ||
			   dasds[dasds_items].devtype == 0x3380 ||
			   dasds[dasds_items].devtype == 0x9345 ||
			   dasds[dasds_items].devtype == 0x9336 ||
			   dasds[dasds_items].devtype == 0x3370)
			{
				if (!strncmp(buf1, "yes", 3))
					dasds[dasds_items].state = UNKNOWN;
				else
					dasds[dasds_items].state = NEW;
				dasds_items++;
				if ((dasds_items % 5) == 0)
					dasds = di_renew (struct dasd, dasds, dasds_items + 5);
			}
	}
	fclose (f);

	if (!update_state ())
		return WANT_ERROR;

	if (dasds_items > 30)
	{
		while (1)
		{
			unsigned int device;

			ret = my_debconf_input ("high", "debian-installer/s390/dasd/choose", &ptr);
			sscanf (ptr, "%4x", &device);

			for (i = 0; i < dasds_items; i++)
				if (device == dasds[i].device)
				{
					dasd_current = &dasds[i];
					break;
				}

			if (!dasd_current)
			{
				ret = my_debconf_input ("high", "debian-installer/s390/dasd/choose_invalid", &ptr);
				if (ret == 10)
					return WANT_BACKUP;
			}
			else
				break;
		}
	}
	else if (dasds_items > 1)
	{
		unsigned int device;

		buf[0] = '\0';
		for (i = 0; i < dasds_items; i++)
		{
			switch (dasds[i].state)
			{
				case NEW:
					ptr = "(new)";
					break;
				case UNFORMATED:
					ptr = "(unformated)";
					break;
				case FORMATED:
					ptr = "(formated)";
					break;
				case READY:
					ptr = "(ready)";
					break;
				default:
					ptr = "(unknown)";
			}
			di_snprintfcat (buf, sizeof (buf), "%04x %s, ", dasds[i].device, ptr);
		}

		debconf_subst (client, "debian-install/s390/dasd/choose_select", "choices", buf);
		ret = my_debconf_input ("high", "debian-install/s390/dasd/choose_select", &ptr);

		if (ret == 10)
			return WANT_BACKUP;
		if (!strcmp (ptr, "Quit"))
			return WANT_QUIT;
		sscanf (ptr, "%4x", &device);

		for (i = 0; i < dasds_items; i++)
			if (device == dasds[i].device)
			{
				dasd_current = &dasds[i];
				break;
			}
	}
	else if (dasds_items)
		dasd_current = dasds;
	return WANT_NEXT;
}

static enum state_wanted confirm (void)
{
	char buf[256], *ptr;
	int ret;

	if (dasd_current->state == NEW)
	{
		snprintf (buf, sizeof (buf), "echo add %04x >/proc/dasd/devices", dasd_current->device);
		ret = di_exec_shell_log (buf);
		if (ret)
			return WANT_ERROR;
		update_state ();
	}

	snprintf (buf, sizeof (buf), "%04x", dasd_current->device);

	switch (dasd_current->state)
	{
		case UNFORMATED:
		case READY:
			debconf_subst (client, "debian-install/s390/dasd/format", "device", buf);
			debconf_set (client, "debian-install/s390/dasd/format", "true");
			ret = my_debconf_input ("medium", "debian-install/s390/dasd/format", &ptr);
			break;
		case FORMATED:
			debconf_subst (client, "debian-install/s390/dasd/format_unclean", "device", buf);
			debconf_set (client, "debian-install/s390/dasd/format_unclean", "false");
			ret = my_debconf_input ("critical", "debian-install/s390/dasd/format_unclean", &ptr);
			break;
		default:
			return WANT_ERROR;
	}

	di_log (DI_LOG_LEVEL_WARNING, "ret: %d, ptr: %s", ret, ptr);
	if (ret == 10 || strncmp (ptr, "true", 4))
		return WANT_BACKUP;
	return WANT_NEXT;
}

static enum state_wanted setup (void)
{
	char buf[256];
	int ret;

	snprintf (buf, sizeof (buf), "dasdfmt -l LX%04x -b 4096 -n %04x -y", dasd_current->device, dasd_current->device);
	ret = di_exec_shell_log (buf);

	if (!ret)
		return WANT_NEXT;

	return WANT_ERROR;
}

static void error (void)
{
	char *ptr;

	my_debconf_input ("high", "debian-install/s390/dasd/error", &ptr);
}

int main(int argc, char *argv[])
{
	di_system_init ("s390-dasd");

	client = debconfclient_new ();
	debconf_capb (client, "backup");

	enum
	{
		BACKUP, GET_CHANNEL,
		CONFIRM, SETUP, ERROR, QUIT
	}
	state = GET_CHANNEL;

	while (1)
	{
		enum state_wanted state_want = WANT_ERROR;

		switch (state)
		{
			case BACKUP:
				return 10;
			case GET_CHANNEL:
				state_want = get_channel ();
				break;
			case CONFIRM:
				state_want = confirm ();
				break;
			case SETUP:
				state_want = setup ();
				break;
			case ERROR:
				error ();
				state_want = WANT_QUIT;
			case QUIT:
				return 0;
		}
		switch (state_want)
		{
			case WANT_NEXT:
				switch (state)
				{
					case GET_CHANNEL:
						state = CONFIRM;
						break;
					case CONFIRM:
						state = SETUP;
						break;
					case SETUP:
						state = GET_CHANNEL;
						break;
					default:
						state = ERROR;
						break;
				}
				break;
			case WANT_BACKUP:
				switch (state)
				{
					case GET_CHANNEL:
						state = BACKUP;
						break;
					case CONFIRM:
					case SETUP:
						state = GET_CHANNEL;
						break;
					default:
						state = ERROR;
						break;
				}
				break;
			case WANT_QUIT:
				state = QUIT;
				break;
			case WANT_ERROR:
				state = ERROR;
		}
	}
}

