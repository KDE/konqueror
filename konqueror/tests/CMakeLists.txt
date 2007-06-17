
include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${KDE4_INCLUDES} )

########### konqviewmgrtest ###############

kde4_automoc(konqviewmgrtest.cpp)

kde4_add_test_executable(konqviewmgrtest konqviewmgrtest.cpp)

target_link_libraries(konqviewmgrtest konq ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

############################################


