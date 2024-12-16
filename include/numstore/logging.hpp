//
// Created by theo on 12/15/24.
//

#ifndef NS_LOGGING_HPP
#define NS_LOGGING_HPP

extern "C"
{

typedef enum {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
} log_type;

#ifndef NLOG

void ns_log(const log_type type, const char *fmt, ...);

// Logs with '\r'
void ns_log_inl(const log_type type, const char *fmt, ...);

// Called once to finish the ns_log_inl call
void ns_log_inl_done();

#else
#define log(...)
#define log_inl(...)
#define log_inl_done(...)
#endif

}

#endif //NS_LOGGING_HPP
