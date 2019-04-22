
#ifndef LIBSYSX_COMMON_H_
#define LIBSYSX_COMMON_H_

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
	#define log_debug(...)    do { printf("debug: " __VA_ARGS__); putchar('\n'); } while(0);
	#define log_info(...)     do { printf("info : " __VA_ARGS__); putchar('\n'); } while(0);
	#define log_warning(...)  do { printf("warn : " __VA_ARGS__); putchar('\n'); } while(0);
	#define log_error(...)    do { printf("error: " __VA_ARGS__); putchar('\n'); } while(0);
#elif defined __GNUC__
	#define log_debug(fmt, args...)    do { printf("debug: " fmt, ## args); putchar('\n'); } while(0);
	#define log_info(fmt, args...)     do { printf("info : " fmt, ## args); putchar('\n'); } while(0);
	#define log_warning(fmt, args...)  do { printf("warn : " fmt, ## args); putchar('\n'); } while(0);
	#define log_error(fmt, args...)    do { printf("error: " fmt, ## args); putchar('\n'); } while(0);
#endif

#endif
