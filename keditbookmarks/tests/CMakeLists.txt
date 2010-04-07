include_directories( ${KDE4_KIO_INCLUDES} .. )
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

###### kbookmarkmodeltest ######

set(kbookmarkmodeltest_SRCS kbookmarkmodeltest.cpp
 ../bookmarkmodel.cpp
 ../commands.cpp
 ../treeitem.cpp
 ../commandhistory.cpp
)

kde4_add_unit_test(kbookmarkmodeltest ${kbookmarkmodeltest_SRCS})

target_link_libraries(kbookmarkmodeltest ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
