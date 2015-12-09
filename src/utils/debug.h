#ifndef _RMY_DEBUG_H
#define _RMY_DEBUG_H 

#define GREEN(text) "\033[0;32m" text "\033[0;0m"
#define RED(text) "\033[0;31m" text "\033[0;0m"
#define YELLOW(text) "\033[0;33m" text "\033[0;0m"
#define BLUE(text) "\033[0;34m" text "\033[0;0m"
#define MAGENTA(text) "\033[0;35m" text "\033[0;0m"
#define BOLD(text) "\033[0;1m" text "\033[0;0m"

#define rmy_log(...) \
	fprintf(stderr, YELLOW("%s:") RED("%d") MAGENTA(":%s" " -> "), \
	 __FILE__, __LINE__, __func__); \
	fprintf(stderr, "\033[0;32m"); \
	fprintf(stderr,   __VA_ARGS__); \
	fprintf(stderr, "\033[0;0m"); \
	fprintf(stderr, "\n")

#define rmy_log_er(...) \
	fprintf(stderr, YELLOW("%s:") RED("%d") MAGENTA(":%s" " -> "), \
	 __FILE__, __LINE__, __func__); \
	fprintf(stderr, "\033[0;31m"); \
	fprintf(stderr,   __VA_ARGS__); \
	fprintf(stderr, "\033[0;0m"); \
	fprintf(stderr, "\n")

#endif