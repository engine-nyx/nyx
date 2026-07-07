#include <ctype.h>
#include <nyx/utils.h>
#include <string.h>

unsigned
str_consume(const char **s, const char *pattern)
{
	size_t len;

	len = strlen(pattern);
	if (strncmp(*s, pattern, len))
		return 0;

	*s += len;
	return len;
}

unsigned
str_ltrim(const char **s)
{
	unsigned count;

	for (count = 0; isblank(**s); ++count)
		++*s;

	return count;
}
