add_subdirectory(tests)

set(kbookmarkmodel_SRCS
  commandhistory.cpp
  commands.cpp
  model.cpp
  treeitem.cpp
  view.cpp
)

kde4_add_library(kbookmarkmodel_private SHARED ${kbookmarkmodel_SRCS})
target_link_libraries(kbookmarkmodel_private ${KDE4_KIO_LIBS})
target_link_libraries(kbookmarkmodel_private LINK_INTERFACE_LIBRARIES ${KDE4_KIO_LIBS})

set_target_properties(kbookmarkmodel_private PROPERTIES
     VERSION ${GENERIC_LIB_VERSION}
     SOVERSION ${GENERIC_LIB_SOVERSION}
)

install(TARGETS kbookmarkmodel_private ${INSTALL_TARGETS_DEFAULT_ARGS})
