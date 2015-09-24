project(buteo-storagechangenotifier)
set(STORAGE_CHANGE_NOTIFIER "hcontacts-changenotifier")

set(STORAGE_CHANGE_NOTIFIER_SRCS
    ContactsChangeNotifier.cpp
    ContactsChangeNotifier.h
    ContactsChangeNotifierPlugin.cpp
    ContactsChangeNotifierPlugin.h
)

add_library(${STORAGE_CHANGE_NOTIFIER} MODULE
    ${STORAGE_CHANGE_NOTIFIER_SRCS}
)

target_link_libraries(${STORAGE_CHANGE_NOTIFIER}
    ${BUTEOSYNCFW_LIBRARIES}
    Qt5::Core
    Qt5::Contacts
)

#set_target_properties(${STORAGE_CHANGE_NOTIFIER}
#    PROPERTIES PREFIX ""
#)

include_directories(
    ${BUTEOSYNCFW_INCLUDE_DIRS}
)

install(TARGETS ${STORAGE_CHANGE_NOTIFIER}
    LIBRARY DESTINATION ${BUTEOSYNCFW_PLUGIN_PATH}
)
