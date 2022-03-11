#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <string_view>

namespace mpcs_51044_log {
    enum LogLevel { INFO, DEBUG, ERROR };

    std::ostream &operator<<(std::ostream &os, LogLevel ll)
    {
        switch (ll) {
            case INFO:
                os << "INFO: ";
                break;
            case DEBUG:
                os << "DEBUG: ";
                break;
            case ERROR:
                os << "ERROR: ";
                break;
            default:
                os.setstate(std::ios_base::failbit);
        }
        return os;
    }

    void print_log(LogLevel ll, std::string_view sv) {
        std::cout << ll << sv << std::endl;
    }
}   // namespace mpcs_51044_log
#endif