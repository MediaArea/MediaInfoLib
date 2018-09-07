AUTOMAKE_OPTIONS = foreign subdir-objects

AM_TESTS_FD_REDIRECT = 9>&2

TESTS = \
    trace.sh \
    s3.sh \
    xml.sh \
    mixml.sh \
    ebucore.sh \
    unicode.sh

check_PROGRAMS = mil_analyze
mil_analyze_SOURCES = \
    Source/analyze.cpp

mil_analyze_CPPFLAGS = -I../../../../Source -I../../../../../ZenLib/Source

mil_analyze_LDFLAGS = ../libmediainfo.la
