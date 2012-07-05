#include <stdio.h>
#include <stdlib.h>

#include "conf_sec.h"

#define HTTP_200_OK_FORMAT		"HTTP/1.1 200 OK\r\nContent-Type: application/configuration\r\nContent-Length: %d\r\n\r\n%s"
#define RAW_CONFIG_MAX_SIZE		(1024*32)
#define ENCODED_CONFIG_MAX_SIZE	(RAW_CONFIG_MAX_SIZE*4)
