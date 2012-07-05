#ifndef RFC2326_H
#define RFC2326_H



static char *rtsp_method[] = { "OPTIONS",
	"DESCRIBE",
	"SETUP",
	"PLAY",
	"PAUSE",
	"TEARDOWN",
	"GET_PARAMETER",
	"SET_PARAMETER",
	"REDIRECT",
	"PING"
};

/* some defines for method field */
#define METHOD_OPTIONS		0
#define METHOD_DESCRIBE		1
#define METHOD_SETUP		2
#define	METHOD_PLAY		3
#define METHOD_PAUSE		4
#define	METHOD_TEARDOWN		5
#define	METHOD_GET_PARAMETER	6
#define METHOD_SET_PARAMETER	7
#define METHOD_REDIRECT		8
#define METHOD_PING		9

#define METHOD_ENTRY_MAX	10

#if 0
static char *rtsp_req_hdr[] = { "Accept",
	"Accept-Encoding",
	"Accept-Language",
	"Authorization",
	"Bandwidth",
	"Blocksize",
	"From",
	"If-Modified-Since",
	"Proxy-Require",
	"Range",
	"Referer",
	"Require",
	"Scale",
	"Session",
	"Speed",
	"Transport",
	"User-Agent"
};


/* some defines for request field */
#define REQ_ACCEPT			0
#define	REQ_ACCEPT_ENCODING		1
#define REQ_ACCEPT_LANGUAGE		2
#define REQ_ACCEPT_AUTHORIZATION	3
#define REQ_BANDWIDTH			4
#define REQ_BLOCKSIZE			5
#define REQ_FROM			6
#define REQ_IF_MODIFIED_SINCE		7
#define REQ_PROXY_REQUIRE		8
#define REQ_RANGE			9
#define REQ_REFERER			10
#define	REQ_REQUIRE			11
#define REQ_SCALE			12
#define REQ_SESSION			13
#define REQ_SPEED			14
#define REQ_SUPPORTED			15
#define REQ_TRANSPORT			16
#define REQ_USER_AGENT			17
#endif

#if 0
char *rtsp_res_hdr[] = { "Accept-Ranges",
	"Location",
	"Proxy-Authenticate",
	"Public",
	"Range",
	"Retry-After",
	"RTP-Info",
	"Scale",
	"Session",
	"Server",
	"Speed",
	"Transport",
	"Unsupported",
	"Vary",
	"WWW-Authenticate"
};
#endif

/* some defines for response field */
#define RES_ACCEPT_RANGES		0
#define RES_LOCATION			1
#define RES_PROXY_AUTHENTICATE		2
#define RES_PUBLIC			3
#define RES_RANGE			4
#define RES_RETRY_AFTER			5
#define RES_RTP_INFO			6
#define RES_SCALE			7
#define RES_SESSION			8
#define RES_SERVER			9
#define RES_SPEED			10
#define RES_TRANSPORT			11
#define RES_UNSUPPORTED			12
#define RES_VARY			13
#define RES_WWW_AUTHENTICATE		14


/* some defines for status code field */
#define STCODE_CONTINUE			100
#define STCODE_OK			200
#define STCODE_CREATED			201
#define STCODE_BAD_REQUEST		400
#define	STCODE_UNAUTHORIZED		401
#define STCODE_NOT_FOUND		404
#define STCODE_METHOD_NOT_ALLOWED	405
#define STCODE_UNSUPPORTED_MEDIA_TYPE	415
#define STCODE_INTERNAL_ERROR		500
#define	STCODE_NOT_IMPLEMENTED		501
#define STCODE_SERVICE_UNAVAILABLE	503
#define STCODE_RTSP_VERSION_NOT_SUPPORTED	505

#if 0
static char *rtsp_entity[] = { "Allow",
	"Content-Base",
	"Content-Encoding",
	"Content-Language",
	"Content-Length",
	"Content-Type",
	"Expires",
	"Last-Modified"
};

/* some defines for entity field */
#define ENTITY_ALLOW			0
#define ENTITY_CONTENT_BASE		1
#define ENTITY_CONTENT_ENCODING		2
#define ENTITY_CONTENT_LANGUAGE		3
#define ENTITY_CONTENT_LENGTH		4
#define ENTITY_CONTENT_LOCATION		5
#define ENTITY_CONTENT_TYPE		6
#define ENTITY_EXPIRES			7
#define ENTITY_LAST_MODIFIED		8
#endif

#endif
