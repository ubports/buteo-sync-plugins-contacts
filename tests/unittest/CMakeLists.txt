# Buteo oop plugin
#add_definitions(-DCLASSNAME=TestContactsClient)
#add_definitions(-DCLASSNAME_H=\"TestContactsClient.h\")
#add_definitions(-DCLIENT_PLUGIN)

# config file

# used on avatar class to avoid network connection during tests
set(PROFILE_TEST_FN "${CMAKE_CURRENT_SOURCE_DIR}/profile-test.xml")
set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config-tests.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/config-tests.h"
               IMMEDIATE @ONLY)

add_executable(test-contact-sync
    TestContactsClient.h
    TestContactsClient.cpp
    TestContactsMain.cpp
    MockAuthenticator.h
    MockAuthenticator.cpp
    MockRemoteSource.h
    MockRemoteSource.cpp
)

target_link_libraries(test-contact-sync
    ${ACCOUNTS_LIBRARIES}
    ${BUTEOSYNCFW_LIBRARIES}
    ${LIBSIGNON_LIBRARIES}
    ubuntu-contact-client
)

include_directories(
    ${buteo-contact-client_SOURCE_DIR}
    ${buteo-contact-google_SOURCE_DIR}
    ${ACCOUNTS_INCLUDE_DIRS}
    ${BUTEOSYNCFW_INCLUDE_DIRS}
    ${LIBSIGNON_INCLUDE_DIRS}
)

qt5_use_modules(test-contact-sync Core Versit Contacts Test)
add_test(test-contact-sync test-contact-sync)
set_tests_properties(test-contact-sync
    PROPERTIES ENVIRONMENT "MSYNCD_LOGGING_LEVEL=10"
)

# Google sync test
add_executable(test-gremotesource
    TestGRemoteSource.cpp
    ${buteo-contact-google_SOURCE_DIR}/GTransport.h
    GTransport.cpp
    ${buteo-contact-google_SOURCE_DIR}/GContactImageUploader.h
    GContactImageUploader.cpp
)

target_link_libraries(test-gremotesource
    ${ACCOUNTS_LIBRARIES}
    ${BUTEOSYNCFW_LIBRARIES}
    ${LIBSIGNON_LIBRARIES}
    ubuntu-contact-client
    googlecontacts-lib
)

qt5_use_modules(test-gremotesource Core Versit Network Contacts Test)
add_test(test-gremotesource test-gremotesource)
set_tests_properties(test-gremotesource
    PROPERTIES ENVIRONMENT "MSYNCD_LOGGING_LEVEL=10"
)

# Google contact parser
add_executable(test-google-contact-parser
    TestGoogleContactParser.cpp
    ${buteo-contact-google_SOURCE_DIR}/GTransport.h
    GTransport.cpp
    ${buteo-contact-google_SOURCE_DIR}/GContactImageUploader.h
    GContactImageUploader.cpp
)

target_link_libraries(test-google-contact-parser
    ${ACCOUNTS_LIBRARIES}
    ${BUTEOSYNCFW_LIBRARIES}
    ${LIBSIGNON_LIBRARIES}
    ubuntu-contact-client
    googlecontacts-lib
)

qt5_use_modules(test-google-contact-parser Core Contacts Xml Test)
add_test(test-google-contact-parser test-google-contact-parser)
set_tests_properties(test-google-contact-parser
    PROPERTIES ENVIRONMENT "MSYNCD_LOGGING_LEVEL=10"
)



