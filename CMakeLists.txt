cmake_minimum_required(VERSION 3.19)
project(harbour-sail-prompt CXX)

find_package (Qt5 COMPONENTS Core Network Qml Gui Quick PrintSupport REQUIRED)

include(FindPkgConfig)
pkg_search_module(SAILFISH sailfishapp REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(LRELEASE_EXECUTABLE lrelease REQUIRED)
set(TS_FILES
    translations/harbour-sail-prompt.ts
    translations/harbour-sail-prompt-de.ts
)
set(QM_FILES)
foreach(TS_FILE ${TS_FILES})
    get_filename_component(TS_NAME ${TS_FILE} NAME_WE)
    set(QM_FILE ${CMAKE_CURRENT_BINARY_DIR}/translations/${TS_NAME}.qm)
    add_custom_command(
        OUTPUT ${QM_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/translations
        COMMAND ${LRELEASE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${TS_FILE} -qm ${QM_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${TS_FILE}
        VERBATIM
    )
    list(APPEND QM_FILES ${QM_FILE})
endforeach()
add_custom_target(translations ALL DEPENDS ${QM_FILES})

add_executable(harbour-sail-prompt
    src/harbour-sail-prompt.cpp
    src/AiChatModel/chat_message.cpp
    src/AiChatModel/chat_message_list_model.cpp
    src/AiChatModel/open_ai_api.cpp
    src/AiChatModel/app_model.cpp
    src/AiChatModel/html_renderer.cpp
    src/AiChatModel/pdf_exporter.cpp
    src/AiChatModel/session_store.cpp
    src/AiChatModel/session_list_model.cpp
)
target_compile_definitions(harbour-sail-prompt PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)
target_include_directories(harbour-sail-prompt PRIVATE
    $<BUILD_INTERFACE:
    ${SAILFISH_INCLUDE_DIRS}
>)
target_link_libraries(harbour-sail-prompt
    Qt5::Quick
    Qt5::Network
    Qt5::PrintSupport
    ${SAILFISH_LDFLAGS}
)

add_executable(test_app_model
    src/test_app_model.cpp
    src/AiChatModel/open_ai_api.cpp
    src/AiChatModel/chat_message.cpp
    src/AiChatModel/html_renderer.cpp
    src/AiChatModel/chat_message_list_model.cpp
)
target_link_libraries(test_app_model
    Qt5::Core
    Qt5::Network
    Qt5::PrintSupport
    Qt5::Gui
)

add_executable(test_session_store
    src/test_session_store.cpp
    src/AiChatModel/session_store.cpp
    src/AiChatModel/session_list_model.cpp
    src/AiChatModel/chat_message.cpp
)
target_link_libraries(test_session_store
    Qt5::Core
)

add_executable(test_pdf_export
    src/test_pdf_export.cpp
    src/AiChatModel/chat_message.cpp
    src/AiChatModel/html_renderer.cpp
    src/AiChatModel/pdf_exporter.cpp
)
target_link_libraries(test_pdf_export
    Qt5::Gui
    Qt5::PrintSupport
)

install(TARGETS harbour-sail-prompt
    RUNTIME DESTINATION bin
)
install(DIRECTORY qml
    DESTINATION share/harbour-sail-prompt
)
install(FILES ${QM_FILES}
    DESTINATION share/harbour-sail-prompt/translations
)
install(FILES harbour-sail-prompt.desktop
    DESTINATION share/applications
)
install(FILES icons/86x86/harbour-sail-prompt.png
    DESTINATION share/icons/hicolor/86x86/apps
)
install(FILES icons/108x108/harbour-sail-prompt.png
    DESTINATION share/icons/hicolor/108x108/apps
)
install(FILES icons/128x128/harbour-sail-prompt.png
    DESTINATION share/icons/hicolor/128x128/apps
)
install(FILES icons/172x172/harbour-sail-prompt.png
    DESTINATION share/icons/hicolor/172x172/apps
)

# Get the other files reachable from the project tree in Qt Creator
FILE(GLOB TsFiles "translations/*.ts")
add_custom_target(distfiles
    SOURCES
        harbour-sail-prompt.desktop
        qml/harbour-sail-prompt.qml
        qml/cover/CoverPage.qml
        qml/pages/FirstPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/SecondPage.qml
        qml/pages/HistoryPage.qml
        rpm/harbour-sail-prompt.changes.in
        rpm/harbour-sail-prompt.changes.run.in
        rpm/harbour-sail-prompt.spec
        ${TsFiles})

# Tell Qt Creator where the application executable(s) would be located on the
# device.
#
# It is not necessary to list other deployables than executables (runtime
# targets) here. The deployment process of Sailfish OS projects is opaque to
# Qt Creator and the information contained in QtCreatorDeployment.txt is only
# used to locate the executable associated with the active run configuration
# on the device in order to run it.
#
# Search the Qt Creator Manual to learn about the QtCreatorDeployment.txt file
# format.
file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
    "${CMAKE_INSTALL_PREFIX}
${CMAKE_BINARY_DIR}/harbour-sail-prompt:bin
")

