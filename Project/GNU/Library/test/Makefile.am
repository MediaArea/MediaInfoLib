AUTOMAKE_OPTIONS = foreign subdir-objects

AM_TESTS_FD_REDIRECT = 9>&2

AM_CXXFLAGS = -std=c++11

TESTS = \
    trace.sh \
    s3.sh \
    xml.sh \
    mixml.sh \
    ebucore.sh \
    unicode.sh \
    filename.sh

check_PROGRAMS = mil_analyze mil_filename
mil_analyze_SOURCES = \
    Source/analyze.cpp

mil_analyze_CPPFLAGS = -I../../../../Source -I../../../../../ZenLib/Source

mil_analyze_LDFLAGS = ../libmediainfo.la

mil_filename_SOURCES = \
    Source/filename.cpp

mil_filename_CPPFLAGS = -I../../../../Source -I../../../../../ZenLib/Source

mil_filename_CXXFLAGS = $(AM_CXXFLAGS) -Wall -Wextra

mil_filename_LDFLAGS = ../libmediainfo.la
