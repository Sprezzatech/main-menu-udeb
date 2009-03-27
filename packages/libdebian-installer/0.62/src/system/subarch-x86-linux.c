/* Some code here borrowed from dmidecode, also GPLed and copyrighted by:
 *   (C) 2000-2002 Alan Cox <alan@redhat.com>
 *   (C) 2002-2005 Jean Delvare <khali@linux-fr.org>
 * I (Colin Watson) copied it in reduced form rather than using dmidecode
 * directly because the d-i initrd is tight on space and this is much
 * smaller (1.8KB versus 46KB, at the time of writing).
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <debian-installer/system/subarch.h>

#define WORD(x) (*(const uint16_t *)(x))
#define DWORD(x) (*(const uint32_t *)(x))

struct dmi_header
{
	uint8_t type;
	uint8_t length;
	uint16_t handle;
};

static int checksum(const uint8_t *buf, size_t len)
{
	uint8_t sum = 0;
	size_t a;

	for (a = 0; a < len; a++)
		sum += buf[a];
	return (sum == 0);
}

/* Copy a physical memory chunk into a memory buffer.
 * This function allocates memory.
 */
static void *mem_chunk(size_t base, size_t len)
{
	void *p;
	int fd;
	size_t mmoffset;
	void *mmp;

	fd = open("/dev/mem", O_RDONLY);
	if (fd == -1)
		return NULL;

	p = malloc(len);
	if (p == NULL)
	{
		close(fd);
		return NULL;
	}

#ifdef _SC_PAGESIZE
	mmoffset = base % sysconf(_SC_PAGESIZE);
#else
	mmoffset = base % getpagesize();
#endif
	/* Please note that we don't use mmap() for performance reasons here,
	 * but to workaround problems many people encountered when trying
	 * to read from /dev/mem using regular read() calls.
	 */
	mmp = mmap(0, mmoffset + len, PROT_READ, MAP_SHARED, fd,
		   base - mmoffset);
	if (mmp == MAP_FAILED)
	{
		free(p);
		close(fd);
		return NULL;
	}

	memcpy(p, mmp + mmoffset, len);

	munmap(mmp, mmoffset + len);

	close(fd);

	return p;
}

static const char *dmi_string(struct dmi_header *dm, uint8_t s)
{
	char *bp = (char *)dm;
	size_t i, len;

	if (s == 0)
		return "Not Specified";

	bp += dm->length;
	while (s > 1 && *bp)
	{
		bp += strlen(bp);
		bp++;
		s--;
	}

	if (!*bp)
		return "<BAD INDEX>";

	/* ASCII filtering */
	len = strlen(bp);
	for (i = 0; i < len; i++)
		if (bp[i] < 32 || bp[i] == 127)
			bp[i] = '.';

	return bp;
}

static char *dmi_table(uint32_t base, uint16_t len, uint16_t num)
{
	uint8_t *buf, *data;
	int i = 0;
	char *ret = NULL;

	buf = mem_chunk(base, len);
	if (buf == NULL)
		return NULL;

	data = buf;
	while (i < num && data + sizeof(struct dmi_header) <= buf + len)
	{
		uint8_t *next;
		struct dmi_header *h = (struct dmi_header *)data;

		/* Stop decoding at end of table marker */
		if (h->type == 127)
			break;

		/* Look for the next handle */
		next = data + h->length;
		while (next - buf + 1 < len && (next[0] != 0 || next[1] != 0))
			next++;
		next += 2;
		/* system-manufacturer */
		if (h->type == 1 && h->length > 0x04)
		{
			ret = strdup(dmi_string(h, data[0x04]));
			break;
		}

		data = next;
		i++;
	}

	free(buf);
	return ret;
}

static char *smbios_decode(uint8_t *buf)
{
	if (checksum(buf, buf[0x05]) &&
	    memcmp(buf + 0x10, "_DMI_", 5) == 0 &&
	    checksum(buf + 0x10, 0x0F))
	{
		return dmi_table(DWORD(buf + 0x18), WORD(buf + 0x16),
				 WORD(buf + 0x1C));
	}

	return NULL;
}

static char *legacy_decode(uint8_t *buf)
{
	if (checksum(buf, 0x0F))
	{
		return dmi_table(DWORD(buf + 0x08), WORD(buf + 0x06),
				 WORD(buf + 0x0C));
	}

	return NULL;
}

static char *dmi_system_manufacturer(void)
{
	uint8_t *buf;
	size_t fp;
	char *ret = NULL;

	buf = mem_chunk(0xF0000, 0x10000);
	if (buf == NULL)
		return NULL;

	for (fp = 0; fp <= 0xFFF0; fp += 16)
	{
		if (memcmp(buf + fp, "_SM_", 4) == 0 && fp <= 0xFFE0)
		{
			ret = smbios_decode(buf + fp);
			if (ret)
				break;
			fp += 16;
		}
		else if (memcmp(buf + fp, "_DMI_", 5) == 0)
		{
			ret = legacy_decode(buf + fp);
			if (ret)
				break;
		}
	}

	free(buf);
	return ret;
}

struct map {
	const char *entry;
	const char *ret;
};

static struct map map_manufacturer[] = {
	{ "Apple Computer, Inc.", "mac" },
	{ "Apple Inc.", "mac" },
	{ NULL, NULL }
};

const char *di_system_subarch_analyze(void)
{
	char *manufacturer = dmi_system_manufacturer();
	int i;

	if (manufacturer)
	{
		for (i = 0; map_manufacturer[i].entry; i++)
		{
			if (!strncasecmp(map_manufacturer[i].entry,
					 manufacturer,
					 strlen(map_manufacturer[i].entry)))
				return map_manufacturer[i].ret;
		}
	}

	return "generic";
}
