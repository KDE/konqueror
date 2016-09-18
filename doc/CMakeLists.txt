find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED DocTools)

add_subdirectory(konqueror)
