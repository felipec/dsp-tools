/*
 * Copyright (C) 2009 Felipe Contreras
 *
 * Author: Felipe Contreras <felipe.contreras@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define SYSLOG

#ifdef SYSLOG
#include <syslog.h>
#endif

#ifdef SYSLOG
static inline int
log_level_to_syslog(unsigned int level)
{
	switch (level) {
	case 0: return LOG_ERR;
	case 1: return LOG_WARNING;
	case 2: return LOG_INFO;
	default: return LOG_DEBUG;
	}
}
#endif

void pr_helper(unsigned int level,
	       const char *file,
	       const char *function,
	       unsigned int line,
	       const char *fmt,
	       ...)
{
	char *tmp;
	va_list args;

	va_start(args, fmt);

	vasprintf(&tmp, fmt, args);

	if (level <= 2) {
#ifdef SYSLOG
		syslog(log_level_to_syslog(level), "%s", tmp);
#else
		printf("%s: %s\n", function, tmp);
#endif
	}

	free(tmp);

	va_end(args);
}
