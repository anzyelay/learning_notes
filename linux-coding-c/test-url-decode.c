bool urldecode(char *buf)
{
	char *c, *p;

	if (!buf || !*buf)
		return true;

#define hex(x) \
	(((x) <= '9') ? ((x) - '0') : \
		(((x) <= 'F') ? ((x) - 'A' + 10) : \
			((x) - 'a' + 10)))

	for (c = p = buf; *p; c++)
	{
		if (*p == '%')
		{
			if (!isxdigit(*(p + 1)) || !isxdigit(*(p + 2)))
				return false;

			*c = (char)(16 * hex(*(p + 1)) + hex(*(p + 2)));

			p += 3;
		}
		else if (*p == '+')
		{
			*c = ' ';
			p++;
		}
		else
		{
			*c = *p++;
		}
	}

	*c = 0;

	return true;
}
