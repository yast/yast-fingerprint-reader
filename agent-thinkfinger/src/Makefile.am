
# Makefile.am for .../agent-thinkfinger/src
#

AM_CXXFLAGS = -DY2LOG=\"agent-thinkfinger\" -Wall 

plugin_LTLIBRARIES = libpy2ag_thinkfinger.la
noinst_LTLIBRARIES = liby2ag_thinkfinger.la

liby2ag_thinkfinger_la_SOURCES =				\
	ThinkFingerAgent.cc					\
	ThinkFingerAgent.h 
liby2ag_thinkfinger_la_LDFLAGS = -version-info 2:0
liby2ag_thinkfinger_la_LIBADD = @AGENT_LIBADD@ -lthinkfinger -L$(libdir)


libpy2ag_thinkfinger_la_SOURCES =				\
        $(liby2ag_thinkfinger_la_SOURCES)			\
        Y2CCThinkFingerAgent.cc
libpy2ag_thinkfinger_la_LDFLAGS = -version-info 2:0
libpy2ag_thinkfinger_la_LIBADD = @AGENT_LIBADD@ -lthinkfinger -L$(libdir)


INCLUDES = -I$(includedir)
