kde4_header()

project(kioslave-about)
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kio_about_PART_SRCS kio_about.cpp )

kde4_automoc(${kio_about_PART_SRCS})

kde4_add_plugin(kio_about ${kio_about_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_about )

target_link_libraries(kio_about  ${KDE4_KIO_LIBS} )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_about )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES about.protocol )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/about
#
#INCLUDES= $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#KDE_CXXFLAGS = -UQT3_SUPPORT
######## Files
#
#kde_module_LTLIBRARIES = kio_about.la
#
#kio_about_la_SOURCES = kio_about.cpp
#kio_about_la_LIBADD = $(LIB_KSYCOCA)
#kio_about_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = kio_about.h
#
#kdelnk_DATA = about.protocol
#kdelnkdir = $(kde_servicesdir)
#
#METASOURCES = AUTO
#
#messages:
#	$(XGETTEXT) *.cpp *.h -o $(podir)/kio_about.pot
