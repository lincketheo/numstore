#pragma once

/////////////////////// Logging

// Regular colors
#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"

// Bold colors
#define BOLD_BLACK "\033[1;30m"
#define BOLD_RED "\033[1;31m"
#define BOLD_GREEN "\033[1;32m"
#define BOLD_YELLOW "\033[1;33m"
#define BOLD_BLUE "\033[1;34m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN "\033[1;36m"
#define BOLD_WHITE "\033[1;37m"

// Reset
#define RESET "\033[0m"

// Internal log function
void i_log_internal (
    const char *prefix,
    const char *color,
    const char *fmt,
    ...)
    __attribute__ ((format (printf, 3, 4)));

// #define LOG_TRACE
#define LOG_DEBUG
#define LOG_INFO
#define LOG_WARN
#define LOG_ERROR
#define LOG_ASSERT
#define LOG_PASSED
#define LOG_FAILURE

// Log macros
#if defined(LOG_TRACE) && !defined(NLOGGING)
#define i_log_trace(...) i_log_internal ("TRACE", BOLD_WHITE, __VA_ARGS__)
#else
#define i_log_trace(...) ((void)0)
#endif

#if defined(LOG_DEBUG) && !defined(NLOGGING)
#define i_log_debug(...) i_log_internal ("DEBUG", BLUE, __VA_ARGS__)
#else
#define i_log_debug(...) ((void)0)
#endif

#if defined(LOG_INFO) && !defined(NLOGGING)
#define i_log_info(...) i_log_internal ("INFO", GREEN, __VA_ARGS__)
#else
#define i_log_info(...) ((void)0)
#endif

#if defined(LOG_WARN) && !defined(NLOGGING)
#define i_log_warn(...) i_log_internal ("WARN", YELLOW, __VA_ARGS__)
#else
#define i_log_warn(...) ((void)0)
#endif

#if defined(LOG_ERROR) && !defined(NLOGGING)
#define i_log_error(...) i_log_internal ("ERROR", RED, __VA_ARGS__)
#else
#define i_log_error(...) ((void)0)
#endif

#if defined(LOG_ASSERT) && !defined(NLOGGING)
#define i_log_assert(...) i_log_internal ("ASSERT", RED, __VA_ARGS__)
#else
#define i_log_assert(...) ((void)0)
#endif

#if defined(LOG_FAILURE) && !defined(NLOGGING)
#define i_log_failure(...) i_log_internal ("FAILURE", BOLD_RED, __VA_ARGS__)
#else
#define i_log_failure(...) ((void)0)
#endif

#if defined(LOG_PASSED) && !defined(NLOGGING)
#define i_log_passed(...) i_log_internal ("PASSED", BOLD_GREEN, __VA_ARGS__)
#else
#define i_log_passed(...) ((void)0)
#endif
