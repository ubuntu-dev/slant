#include <sys/queue.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgijson.h>
#include <ksql.h>

#include "extern.h"
#include "db.h"
#include "json.h"

enum	page {
	PAGE_INDEX,
	PAGE__MAX
};

static const char *const pages[PAGE__MAX] = {
	"index", /* PAGE_INDEX */
};

/*
 * Fill out generic headers then start the HTTP document body (no more
 * headers after this point!)
 */
static void
http_open(struct kreq *r, enum khttp code)
{

	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[code]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE], 
		"%s", kmimetypes[r->mime]);
	khttp_body(r);
}

static void
sendindex(struct kreq *r, const struct record_q *q)
{
	struct kjsonreq	 req;
	const struct record *rr;

	http_open(r, KHTTP_200);
	kjson_open(&req, r);
	kjson_obj_open(&req);

	kjson_arrayp_open(&req, "qmin");
	TAILQ_FOREACH(rr, q, _entries)
		if (INTERVAL_byqmin == rr->interval) {
			kjson_obj_open(&req);
			json_record_data(&req, rr);
			kjson_obj_close(&req);
		}
	kjson_array_close(&req);
	kjson_arrayp_open(&req, "min");
	TAILQ_FOREACH(rr, q, _entries)
		if (INTERVAL_bymin == rr->interval) {
			kjson_obj_open(&req);
			json_record_data(&req, rr);
			kjson_obj_close(&req);
		}
	kjson_array_close(&req);
	kjson_arrayp_open(&req, "hour");
	TAILQ_FOREACH(rr, q, _entries)
		if (INTERVAL_byhour == rr->interval) {
			kjson_obj_open(&req);
			json_record_data(&req, rr);
			kjson_obj_close(&req);
		}
	kjson_array_close(&req);

	kjson_obj_close(&req);
	kjson_close(&req);
}

int
main(void)
{
	struct kreq	 r;
	enum kcgi_err	 er;
	struct record_q	*rq;

	if (-1 == pledge("stdio rpath "
	    "cpath wpath flock fattr proc", NULL)) {
		kutil_warn(NULL, NULL, "pledge");
		return EXIT_FAILURE;
	}

	er = khttp_parse(&r, NULL, 0, pages, PAGE__MAX, PAGE_INDEX);

	if (KCGI_OK != er) {
		kutil_warnx(NULL, NULL, "%s", kcgi_strerror(er));
		return EXIT_FAILURE;
	}

	/*
	 * Front line of defence: make sure we're a proper method, make
	 * sure we're a page, make sure we're a JSON file.
	 */

	if (KMETHOD_GET != r.method) {
		http_open(&r, KHTTP_405);
		khttp_free(&r);
		return EXIT_SUCCESS;
	} else if (PAGE__MAX == r.page || 
	           KMIME_APP_JSON != r.mime) {
		http_open(&r, KHTTP_404);
		khttp_free(&r);
		return EXIT_SUCCESS;
	}

	if (NULL == (r.arg = db_open("/data/slant.db"))) {
		khttp_free(&r);
		return EXIT_SUCCESS;
	}

	if (-1 == pledge("stdio", NULL)) {
		kutil_warn(NULL, NULL, "pledge");
		db_close(r.arg);
		khttp_free(&r);
		return EXIT_FAILURE;
	}

	db_role(r.arg, ROLE_consume);
	rq = db_record_list_lister(r.arg);
	sendindex(&r, rq);
	db_record_freeq(rq);
	db_close(r.arg);
	khttp_free(&r);
	return EXIT_SUCCESS;
}