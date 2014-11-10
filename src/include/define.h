#ifndef _MAID_INTERNAL_DEFINE_H_
#define _MAID_INTERNAL_DEFINE_H_

#ifdef DEBUG
#include <stdio.h>
#define WARN(format, args...) do { fprintf(stdout, "[WARN]"format"\n", ##args); } while (0)
#define ERROR(format, args...) do { fprintf(stdout, "[ERROR]"format"\n", ##args); } while (0)
#define INFO(format, args...)  do { fprintf(stdout, "[INFO]"format"\n", ##args); } while (0)
#define ASSERT(condition, format, args...) do { if ( !(condition) ) { fprintf(stdout, "[ASSERT]"format"\n", ##args); exit(-1);} } while(0)

#else
#define ERRO(format, ...)
#define WARN(format, ...)
#define INFO(format, ...)
#define ASSERT(con, format, ...)
#endif

#define ERROR_OUT_OF_LENGTH -1
#define ERROR_LACK_DATA     -2
#define ERROR_BUSY          -3
#define ERROR_PARSE_FAILED  -4
#define ERROR_OTHER         -5

#define CONTROLLERMETA_MAX_LENGTH 10000
#define RESERVED_METHOD_CONNECT "Connect"
#define RESERVED_METHOD_DISCONNECT "Disconnect"

#endif /* _MAID_INTERNAL_DEFINE_H_ */
