set(SERVICE_FILES
    buteo-google-contacts.service
)
set(APPLICATION_FILES
    buteo-address-book-app.application
)

install(FILES ${SERVICE_FILES}
    DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/accounts/services
)

install(FILES ${APPLICATION_FILES}
    DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/accounts/applications
)
