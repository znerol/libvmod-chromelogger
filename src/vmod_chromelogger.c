#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"

#include "vcc_if.h"

#define DEBUG 1

#ifdef DEBUG                    // To print diagnostics to the error log
#define _DEBUG 1                // enable through gcc -DDEBUG
#else
#define _DEBUG 0
#endif

#define MAX_DATA 1024
#define MAX_BT 256

const char * const typeNames[] = {
    "log",
    "warn",
    "error",
    "info",
    "group",
    "groupEnd",
    "groupCollapsed",
    "table"
};

enum type_e {
    LOG,
    WARN,
    ERROR,
    INFO,
    GROUP,
    GROUPEND,
    GROUPCOLLAPSED,
    TABLE
};


struct entry {
	char data[MAX_DATA];
        char backtrace[MAX_BT];
        enum type_e type;
	VTAILQ_ENTRY(entry) list;
};
VTAILQ_HEAD(, entry) logentries = VTAILQ_HEAD_INITIALIZER(logentries);

void vmod_VSB_quote_real(struct vsb *s, const char *p);
void vmod_VSB_base64_encode(struct vsb *s, const char *p, ssize_t len);

/**
 * Add a new log message.
 */
void
vmod_log(struct sess *sp, const char *s) {
        int c;
        struct entry *newentry;

        // Ignore empty lines
	if (strlen(s) == 0) {
		return;
	}

        newentry = (struct entry*)WS_Alloc(sp->ws, sizeof(struct entry));
	AN(newentry);

        strncpy(newentry->data, s, MAX_DATA);
        newentry->data[MAX_DATA-1] = '\0';
        strcpy(newentry->backtrace, "FIXME");
        newentry->type = LOG;

	VTAILQ_INSERT_TAIL(&logentries, newentry, list);
}

/**
 *
 */
const char * __match_proto__()
vmod_collect(struct sess *sp) {
        struct entry *e;
        struct entry *e2;
	struct vsb *json;
	struct vsb *output;
	unsigned v, u;
	char *p;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);

        if (VTAILQ_EMPTY(&logentries)) {
                return NULL;
        }

	u = WS_Reserve(sp->wrk->ws, 0);
	p = sp->wrk->ws->f;

	json = VSB_new_auto();
	AN(json);

	VSB_cpy(json, "{\"version\": \"0.2\",\"columns\": [\"log\", \"backtrace\", \"type\"],\"rows\": [");

	VTAILQ_FOREACH_SAFE(e, &logentries, list, e2) {
		VSB_cat(json, "[[");
		vmod_VSB_quote_real(json, e->data);
		VSB_cat(json, "],");
		vmod_VSB_quote_real(json, e->backtrace);
		VSB_cat(json, ",\"");
		VSB_cat(json, typeNames[e->type]);
                VSB_cat(json, "\"],");
		VTAILQ_REMOVE(&logentries, e, list);
	}
        // Remove last comma
        json->s_len--;
	VSB_cat(json, "]}");
	VSB_finish(json);

        // Base64 encode
	output = VSB_new_auto();
	AN(output);

        vmod_VSB_base64_encode(output, VSB_data(json), VSB_len(json));
	VSB_finish(output);

	v = VSB_len(output);
	strcpy(p, VSB_data(output));

	VSB_delete(output);
	VSB_delete(json);

	v++;
	if (v > u) {
			WS_Release(sp->wrk->ws, 0);
			return (NULL);
	}
	WS_Release(sp->wrk->ws, v);
	return (p);
}

/*
 * Quote a string
 */
void
vmod_VSB_quote_real(struct vsb *s, const char *p)
{
	const char *q;
        int len = strlen(p);

	(void)VSB_putc(s, '"');
	for (q = p; q < p + len; q++) {
		switch (*q) {
		case ' ':
			(void)VSB_putc(s, *q);
			break;
		case '\\':
		case '"':
			(void)VSB_putc(s, '\\');
			(void)VSB_putc(s, *q);
			break;
		case '\n':
			(void)VSB_cat(s, "\\n");
			break;
		case '\r':
			(void)VSB_cat(s, "\\r");
			break;
		case '\t':
			(void)VSB_cat(s, "\\t");
			break;
		default:
			if (isgraph(*q))
				(void)VSB_putc(s, *q);
			else
				(void)VSB_printf(s, "\\%o", *q & 0xff);
			break;
		}
	}
	(void)VSB_putc(s, '"');
}

void
vmod_VSB_base64_encode(struct vsb *s, const char *p, ssize_t len) {
        const static char* b64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        ssize_t i;

        if (len == -1) {
                len = strlen(p);
        }

        for (i = 0; i < len - 2; i += 3) {
                VSB_putc(s, b64[p[i] >> 2]);
                VSB_putc(s, b64[((0x3 & p[i]) << 4) + (p[i+1] >> 4)]);
                VSB_putc(s, b64[((0xf & p[i+1]) << 2) + (p[i+2] >> 6)]);
                VSB_putc(s, b64[0x3f & p[i+2]]);
        }

        if (len % 3 == 2) {
                VSB_putc(s, b64[p[i] >> 2]);
                VSB_putc(s, b64[((0x3 & p[i]) << 4) + (p[i+1] >> 4)]);
                VSB_putc(s, b64[((0xf & p[i+1]) << 2)]);
                VSB_putc(s, '=');
        }

        if (len % 3 == 1) {
                VSB_putc(s, b64[p[i] >> 2]);
                VSB_putc(s, b64[((0x3 & p[i]) << 4)]);
                VSB_putc(s, '=');
                VSB_putc(s, '=');
        }
}
