project(buteo-contact-client)

set(UBUNTU_CONTACTS_CLIENT ubuntu-contact-client)
set(UBUNTU_CONTACTS_CLIENT_SRCS
    UAbstractRemoteSource.h
    UAbstractRemoteSource.cpp
    UAuth.cpp
    UAuth.h
    UContactsBackend.cpp
    UContactsBackend.h
    UContactsClient.cpp
    UContactsClient.h
    UContactsCustomDetail.cpp
    UContactsCustomDetail.h
)


add_library(${UBUNTU_CONTACTS_CLIENT} STATIC
    ${UBUNTU_CONTACTS_CLIENT_SRCS}
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${ACCOUNTS_INCLUDE_DIRS}
    ${BUTEOSYNCFW_INCLUDE_DIRS}
    ${LIBSIGNON_INCLUDE_DIRS}
)

target_link_libraries(${UBUNTU_CONTACTS_CLIENT}
    Qt5::Core
    Qt5::Network
    Qt5::DBus
    Qt5::Contacts
    ${ACCOUNTS_LIBRARIES}
    ${BUTEOSYNCFW_LIBRARIES}
    ${LIBSIGNON_LIBRARIES}
)
