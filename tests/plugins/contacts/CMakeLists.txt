set(QCONTACTS_BACKEND qtcontacts_mock)

set(QCONTACTS_BACKEND_SRCS
    qcontactmemorybackend_p.h
    qcontactmemorybackend.cpp
)

add_library(${QCONTACTS_BACKEND} MODULE
    ${QCONTACTS_BACKEND_SRCS}
)

qt5_use_modules(${QCONTACTS_BACKEND} Core Contacts)
