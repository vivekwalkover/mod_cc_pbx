include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_cc_pbx

mod_LTLIBRARIES = mod_cc_pbx.la
mod_cc_pbx_la_SOURCES  = mod_cc_pbx.c fs_dbh.c cc_pbx_features.c cc_pbx_func.c
#mod_cc_pbx_la_SOURCES  = mod_cc_pbx.c fs_dbh.c
mod_cc_pbx_la_CFLAGS   = $(AM_CFLAGS)
mod_cc_pbx_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_cc_pbx_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
