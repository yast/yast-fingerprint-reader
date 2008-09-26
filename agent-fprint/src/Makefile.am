
# Makefile.am for .../agent-fprint/src
#

AM_CXXFLAGS = -DY2LOG=\"agent-fprint\" -Wall

plugin_LTLIBRARIES = libpy2ag_fprint.la
noinst_LTLIBRARIES = liby2ag_fprint.la

liby2ag_fprint_la_SOURCES =				\
	FPrintAgent.cc					\
	FPrintAgent.h 
liby2ag_fprint_la_LDFLAGS = -version-info 2:0
liby2ag_fprint_la_LIBADD = @AGENT_LIBADD@ -lfprint


libpy2ag_fprint_la_SOURCES =				\
        $(liby2ag_fprint_la_SOURCES)			\
        Y2CCFPrintAgent.cc
libpy2ag_fprint_la_LDFLAGS = -version-info 2:0
libpy2ag_fprint_la_LIBADD = @AGENT_LIBADD@ -lfprint


INCLUDES = -I$(includedir)
