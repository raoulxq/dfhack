include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set ( dfoffsetedit_SRCS
    dfedit.cpp
    memxmlModel.cpp
    main.cpp
)

SET ( dfoffsetedit_UI
    gui/main.ui
)

SET( dfoffsetedit_RCS
    gui/resources.qrc
)

# this command will generate rules that will run rcc on all files from dfoffsetedit_RCS
# in result dfoffsetedit_RC_SRCS variable will contain paths to files produced by rcc
QT4_ADD_RESOURCES( dfoffsetedit_RC_SRCS ${dfoffsetedit_RCS} )

QT4_WRAP_UI(dfoffsetedit_UI_h ${dfoffsetedit_UI})
qt4_automoc(${dfoffsetedit_SRCS})

add_executable(dfoffsetedit ${dfoffsetedit_SRCS} ${dfoffsetedit_RC_SRCS} ${dfoffsetedit_UI_h})
target_link_libraries(dfoffsetedit ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} )