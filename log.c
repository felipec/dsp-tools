/*
 * Copyright (C) 2009 Nokia Corporation.
 *
 * Author: Felipe Contreras <felipe.contreras@nokia.com>
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef SYSLOG
#include <syslog.h>
#endif

unsigned int debug_level = 1;
const char *debug_id = "log";

#define EXTRA_DEBUG

static inline const char *
log_level_to_string(unsigned int level)
{
    switch (level)
    {
        case 0: return "error"; break;
        case 1: return "warning"; break;
        case 2: return "info"; break;
        default: return NULL; break;
    }
}

void pr_helper(unsigned int level,
	       const char *file,
	       const char *function,
	       unsigned int line,
	       const char *fmt,
	       ...)
{
    char *tmp;
    va_list args;

    if (level < debug_level)
	    return;

    va_start (args, fmt);

    vasprintf (&tmp, fmt, args);

#ifdef EXTRA_DEBUG
    fprintf(stderr, "%s %s:%d:%s() %s\n",
	    log_level_to_string (level),
	    file, line, function,
	    tmp);
#else
    fprintf(stderr, "%s: %s\n",
	    log_level_to_string (level),
	    tmp);
#endif

#ifdef SYSLOG
    {
	    int syslog_level;
	    switch(level) {
		    case 0:
		    syslog_level = LOG_ERR;
		    case 1:
		    syslog_level = LOG_WARNING;
		    case 2:
		    syslog_level = LOG_INFO;
		    default:
		    syslog_level = LOG_DEBUG;
	    }
	    syslog(syslog_level, "%s", tmp);
    }
#endif

    free (tmp);

    va_end (args);
}
