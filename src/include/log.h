/*
;This is a simple logger class that can be used to debug or log issues
*/

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <exception>

#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "ioutil.h"

class Log {
    public:
        std::ofstream *file;
        ioutil::MultiWriter mw;

        Log(const char* f, bool use_stdout) {
            file = new std::ofstream;
            file->open(f, std::ios::out | std::ios::app);
            mw.add(file);

            if (use_stdout) {
                mw.add(&std::cout);
            }
        }

        void L(const char* format, ...) {
            va_list argptr;
            char buffer[2048];

            va_start(argptr, format);
            vsprintf(buffer, format, argptr);

            // Format a time string
            char timestring[8];
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            strftime(timestring, sizeof(timestring) - 1, "%H:%M", t);

            // Write it out to the optional files
            mw.write("[", 1);
            mw.write(timestring, strlen(timestring));
            mw.write("] ", 2);
            mw.write(buffer, strlen(buffer));
            mw.write("\n", 1);

            va_end(argptr);
        }

        ~Log() {
            file->close();
        }
};

class Exception: public std::exception {
    private:
        std::string err;

        virtual const char* what() const throw() {
            return this->err.c_str();
        }

    public:
        Exception(std::string err) {
            this->err = err;
        }
};
