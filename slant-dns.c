/*	$Id$ */
/*
 * Copyright (c) 2018 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/queue.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <err.h>
#include <netdb.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"
#include "slant.h"

/*
 * Parse the url in n->url into its component parts.
 * Returns zero on error, non-zero on success.
 */
void
dns_parse_url(struct out *out, struct node *n)
{
	char		*cp;
	const char 	*s = n->url;

 	n->addrs.https = 0;
	n->addrs.port = 80;

	if (0 == strncasecmp(s, "https://", 8)) {
	 	n->addrs.https = 1;
		n->addrs.port = 443;
		s += 8;
	} else if (0 == strncasecmp(s, "http://", 7))
		s += 7;

	if (NULL == (n->host = strdup(s)))
		err(EXIT_FAILURE, NULL);

	for (cp = n->host; '\0' != *cp; cp++)
		if ('/' == *cp) {
			if (NULL == (n->path = strdup(cp)))
				err(EXIT_FAILURE, NULL);
			*cp = '\0';
			for (cp = n->path; '\0' != *cp; cp++)
				if ('?' == *cp || '#' == *cp) {
					*cp = '\0';
					break;
				}
			break;
		} else if ('?' == *cp || '#' == *cp) {
			*cp = '\0';
			break;
		}

	if (NULL == n->path &&
	    NULL == (n->path = strdup("/")))
		err(EXIT_FAILURE, NULL);
}

/*
 * This is a modified version of host_dns in config.c of OpenBSD's ntpd.
 */
/*
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
int
dns_resolve(struct out *out, const char *host, struct dns *vec)
{
	struct addrinfo	 hints, *res0, *res;
	struct sockaddr	*sa;
	int		 error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM; /* DUMMY */

	error = getaddrinfo(host, NULL, &hints, &res0);

	xdbg(out, "DNS resolving: %s", host);

	if (error == EAI_AGAIN || error == EAI_NONAME) {
		xwarnx(out, "DNS resolve error: %s: %s", 
			host, gai_strerror(error));
		return 0;
	} else if (error) {
		xwarnx(out, "DNS parse error: %s: %s",
			host, gai_strerror(error));
		return 0;
	}

	for (vec->addrsz = 0, res = res0;
	     NULL != res && vec->addrsz < MAX_SERVERS_DNS;
	     res = res->ai_next) {
		if (res->ai_family != AF_INET &&
		    res->ai_family != AF_INET6)
			continue;

		sa = res->ai_addr;
		if (AF_INET == res->ai_family) {
			vec->addrs[vec->addrsz].family = 4;
			inet_ntop(AF_INET,
				&(((struct sockaddr_in *)sa)->sin_addr),
				vec->addrs[vec->addrsz].ip, 
				INET6_ADDRSTRLEN);
		} else {
			vec->addrs[vec->addrsz].family = 6;
			inet_ntop(AF_INET6,
				&(((struct sockaddr_in6 *)sa)->sin6_addr),
				vec->addrs[vec->addrsz].ip, 
				INET6_ADDRSTRLEN);
		}
		
		xdbg(out, "DNS resolved: %s: %s",
			host, vec->addrs[vec->addrsz].ip);
		vec->addrsz++;
	}

	freeaddrinfo(res0);
	return 1;
}
