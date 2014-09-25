#ifdef DEBUG
#define WARN(format, args...) do { fprintf(stdout, "[WARN]"format"\n", ##args); } while (0)
#define ERROR(format, args...) do { fprintf(stdout, "[ERROR]"format"\n", ##args); } while (0)
#define INFO(format, args...)  do { fprintf(stdout, "[INFO]"format"\n", ##args); } while (0)
#define ASSERT(condition, format, args...) do { if ( !(condition) ) { fprintf(stdout, "[ASSERT]"format"\n", ##args); } } while(0)

#else
#define ERRO(format, ...)
#define WARN(format, ...)
#define INFO(format, ...)
#define ASSERT(con, format, ...)
#endif
