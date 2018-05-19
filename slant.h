#ifndef SLANT_H
#define SLANT_H

struct	source {
	int	 family; /* 4 (PF_INET) or 6 (PF_INET6) */
	char	 ip[INET6_ADDRSTRLEN]; /* IPV4 or IPV6 address */
};

#define MAX_SERVERS_DNS 8

struct	dns {
	size_t	 	 addrsz; /* num addrs (<= MAX_SERVERS_DNS) */
	struct source	 addrs[MAX_SERVERS_DNS]; /* ip addresses */
	short	 	 port; /* port */
	int		 https; /* non-zero if https, else zero */
	size_t		 curaddr; /* current working address */
};

struct	recset {
	struct record	*byqmin;
	size_t		 byqminsz;
	struct record	*bymin;
	size_t		 byminsz;
	struct record	*byhour;
	size_t		 byhoursz;
};

enum	state {
	STATE_STARTUP = 0,
	STATE_RESOLVING,
	STATE_CONNECT_WAITING,
	STATE_CONNECT_READY,
	STATE_CONNECT,
	STATE_WRITE,
	STATE_READ,
	STATE_WAITING,
	STATE_DONE,
};

struct	xfer {
	char		*wbuf; /* write buffer for http */
	size_t		 wbufsz; /* amount left to write */
	size_t		 wbufpos; /* write position in wbuf */
	char		*rbuf; /* read buffer for http */
	size_t		 rbufsz; /* amount read over http */
	struct sockaddr_storage ss; /* socket */
	struct pollfd	*pfd; /* pollfd descriptor */
};

struct	node {
	enum state	 state; /* state of node */
	char		*url; /* full URL for connect */
	char		*host; /* just hostname of connect */
	char		*path; /* path of connect */
	struct xfer	 xfer; /* transfer information */
	struct dns	 addrs; /* all possible IP addresses */
	time_t		 waitstart; /* wait period start */
	struct recset	*recs; /* results */
	int		 dirty; /* new results */
};

/* All of this is from json.h. */

enum	json_type_e {
	json_type_string,
	json_type_number,
	json_type_object,
	json_type_array,
	json_type_true,
	json_type_false,
	json_type_null
};

struct 	json_value_s {
	void 		*payload;
	enum json_type_e type;
};

struct	json_number_s {
	const char *number;
	size_t 	number_size;
};

struct 	json_array_element_s {
	struct json_value_s *value;
	struct json_array_element_s *next;
};

struct	json_array_s {
	struct json_array_element_s *start;
	size_t 	 length;
};

struct 	json_string_s {
	const char *string;
	size_t	 string_size;
};

struct 	json_object_element_s {
	struct json_string_s *name;
	struct json_value_s *value;
	struct json_object_element_s *next;
};

struct	json_object_s {
	struct json_object_element_s *start;
	size_t 	 length;
};

__BEGIN_DECLS

int	 dns_parse_url(struct node *);
int	 dns_resolve(const char *, struct dns *);
int	 http_init_connect(struct node *);
int	 http_connect(struct node *);
int	 http_write(struct node *n);
int	 http_read(struct node *n);
void 	 draw(const struct node *n, size_t sz);
struct json_value_s *json_parse(const void *, size_t);
int 	 jsonobj_parse(struct node *n, const char *, size_t);

__END_DECLS

#endif /* SLANT_H */