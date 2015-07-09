project(buteo-contact-google)
set(GOOGLE_CONTACTS_CLIENT googlecontacts-client)
set(GOOGLE_CONTACTS_LIB    googlecontacts-lib)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${buteo-contact-client_SOURCE_DIR}
    ${ACCOUNTS_INCLUDE_DIRS}
    ${BUTEOSYNCFW_INCLUDE_DIRS}
    ${LIBSIGNON_INCLUDE_DIRS}
)

set(GOOGLE_CONTACTS_LIB_SRCS
    atom_global.h
    buteo-gcontact-plugin_global.h
    buteosyncfw_p.h
    GConfig.h
    GConfig.cpp
    GContactAtom.h
    GContactAtom.cpp
    GContactImageDownloader.h
    GContactImageDownloader.cpp
    GContactImageUploader.h
    GContactImageUploader.cpp
    GContactStream.h
    GContactStream.cpp
    GRemoteSource.h
    GRemoteSource.cpp
)

add_library(${GOOGLE_CONTACTS_LIB} STATIC
    ${GOOGLE_CONTACTS_LIB_SRCS}
)

target_link_libraries(${GOOGLE_CONTACTS_LIB}
    ${ACCOUNTS_LIBRARIES}
    ${BUTEOSYNCFW_LIBRARIES}
    ${LIBSIGNON_LIBRARIES}
    ubuntu-contact-client
)

qt5_use_modules(${GOOGLE_CONTACTS_LIB} Core Network Contacts)

# Buteo oop plugin
add_definitions(-DCLASSNAME=GContactsClient)
add_definitions(-DCLASSNAME_H=\"GContactsClient.h\")
add_definitions(-DCLIENT_PLUGIN)
set(GOOGLE_CONTACT_BUTEO_SRCS
    ${BUTEOSYNCFW_INCLUDEDIR}/ButeoPluginIfaceAdaptor.h
    ${BUTEOSYNCFW_INCLUDEDIR}/PluginCbImpl.h
    ${BUTEOSYNCFW_INCLUDEDIR}/PluginServiceObj.h
    ${BUTEOSYNCFW_INCLUDEDIR}/ButeoPluginIfaceAdaptor.cpp
    ${BUTEOSYNCFW_INCLUDEDIR}/PluginCbImpl.cpp
    ${BUTEOSYNCFW_INCLUDEDIR}/PluginServiceObj.cpp
    ${BUTEOSYNCFW_INCLUDEDIR}/plugin_main.cpp
)

set(GOOGLE_CONTACTS_CLIENT_SRCS
    GContactsClient.h
    GContactsClient.cpp
    # real implementation
    GTransport.h
    GTransport.cpp
    ${GOOGLE_CONTACT_BUTEO_SRCS}
)

add_executable(${GOOGLE_CONTACTS_CLIENT}
    ${GOOGLE_CONTACTS_CLIENT_SRCS}
)

target_link_libraries(${GOOGLE_CONTACTS_CLIENT}
    ${ACCOUNTS_LIBRARIES}
    ${BUTEOSYNCFW_LIBRARIES}
    ${LIBSIGNON_LIBRARIES}
    ${GOOGLE_CONTACTS_LIB}
    ubuntu-contact-client
)

qt5_use_modules(${GOOGLE_CONTACTS_CLIENT} Core DBus Network Contacts)

install(TARGETS ${GOOGLE_CONTACTS_CLIENT}
    RUNTIME DESTINATION "${BUTEOSYNCFW_PLUGIN_PATH}/oopp"
)

add_subdirectory(xmls)


