# Makefile.am for fingerprint-reader/src

yncludedir = @yncludedir@/fingerprint-reader

client_DATA =			\
	fingerprint-reader.ycp	\
	users_plugin_fingerprint_reader.ycp

ynclude_DATA =			\
	wizards.ycp		\
	dialogs.ycp		\
	helps.ycp

desktop_DATA =			\
	fingerprint-reader.desktop

module_DATA =			\
	FingerprintReader.ycp	\
	UsersPluginFingerprintReader.pm

# create a symlink for local build, #145327
fingerprint-reader:
	ln -sf . $@
ycpchook = fingerprint-reader

EXTRA_DIST = $(client_DATA) $(ynclude_DATA) $(module_DATA) $(desktop_DATA)

include $(top_srcdir)/Makefile.am.common
