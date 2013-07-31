# Sources for fingerprint-reader

module_DATA = \
  modules/FingerprintReader.rb \
  modules/UsersPluginFingerprintReader.pm

client_DATA = \
  clients/fingerprint-reader.rb \
  clients/users_plugin_fingerprint_reader.rb

yncludedir = @yncludedir@/fingerprint-reader
ynclude_DATA = \
  include/fingerprint-reader/wizards.rb \
  include/fingerprint-reader/dialogs.rb \
  include/fingerprint-reader/helps.rb

scrconf_DATA = \
  scrconf/fprint.scr

desktop_DATA = \
  desktop/fingerprint-reader.desktop

EXTRA_DIST = $(module_DATA) $(client_DATA) $(ynclude_DATA) $(scrconf_DATA) $(desktop_DATA)

include $(top_srcdir)/Makefile.am.common