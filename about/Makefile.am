## Makefile.am of kdebase/kioslave/about

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
KDE_CXXFLAGS = -UQT3_SUPPORT
####### Files

kde_module_LTLIBRARIES = kio_about.la

kio_about_la_SOURCES = kio_about.cpp
kio_about_la_LIBADD = $(LIB_KSYCOCA)
kio_about_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = kio_about.h

kdelnk_DATA = about.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/kio_about.pot
