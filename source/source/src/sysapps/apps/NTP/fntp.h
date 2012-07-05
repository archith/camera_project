#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>

/* SNTP Protocol Packet Reference 
                    1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     0+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
     4+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root Delay                           |
     8+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root Dispersion                         |
    12+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                      |
    16+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Reference Timestamp (64)                    |
      |                                                               |
    24+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Originate Timestamp (64)                    |
      |                                                               |
    32+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Receive Timestamp (64)                     |
      |                                                               |
    40+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Transmit Timestamp (64)                    |
      |                                                               |
    48+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       -- Authentication parms removed since we don't support them --
*/

#define NTP_POS_FLAGS				0
#define NTP_POS_STRATUM				1
#define NTP_POS_POLL				2
#define NTP_POS_PRECISION			3
#define NTP_POS_ROOT_DELAY			4
#define NTP_POS_ROOT_DISPERSION			8
#define NTP_POS_REFID				12
#define NTP_POS_REF_TS				16
#define NTP_POS_ORIG_TS				24
#define NTP_POS_RX_TS				32
#define NTP_POS_TX_TS				40

// Constants used in NTP<->TIMEVAL conversion
#define UNITS_PER_S 1000000.
#define NTP_PER_S 4294967296.

// NTP Fractional is defined as number of 2^32 fractions within a second
// Clock is using uSec
// This factor can be used as multiplcand / divisor to convert between formats
#define NTP_OFFSET 2.32830643654E-04

// Default port # for SNTP operations
#define SNTP_DEFAULT_PORT 123

// System clock EPOCHs at 1 JAN 1970, NTP EPOCHs at 1 JAN 1900
// The epoch offset is the difference between these two dates, in seconds
#define EPOCH_OFFSET 0x83AA7E80l

// Packet Sizes
#define SNTP_PACKET_SIZE 48			// 48b standard non auth NTP packet size
#define SNTP_RX_BUFFER   1024			// 1024b to allow for oversized packet RX

//int NTP_update(char *serv);
