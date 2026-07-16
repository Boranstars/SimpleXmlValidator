cmake_minimum_required(VERSION 3.30)
project(SimpleXmlValidator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(SIMPLE_XML_VALIDATOR_ENABLE_ASAN "启用 AddressSanitizer（含 LeakSanitizer）" OFF)
option(SIMPLE_XML_VALIDATOR_ENABLE_UBSAN "启用 UndefinedBehaviorSanitizer" OFF)

add_library(SimpleXmlValidatorSanitizers INTERFACE)

# Use sanitizer flags only if the compiler supports them
if (SIMPLE_XML_VALIDATOR_ENABLE_ASAN OR SIMPLE_XML_VALIDATOR_ENABLE_UBSAN)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(SIMPLE_XML_VALIDATOR_SANITIZER_FLAGS)

        if (SIMPLE_XML_VALIDATOR_ENABLE_ASAN)
            list(APPEND SIMPLE_XML_VALIDATOR_SANITIZER_FLAGS
                    -fsanitize=address
                    -fno-omit-frame-pointer)
        endif ()

        if (SIMPLE_XML_VALIDATOR_ENABLE_UBSAN)
            list(APPEND SIMPLE_XML_VALIDATOR_SANITIZER_FLAGS
                    -fsanitize=undefined)
        endif ()

        target_compile_options(SimpleXmlValidatorSanitizers INTERFACE
                ${SIMPLE_XML_VALIDATOR_SANITIZER_FLAGS})
        target_link_options(SimpleXmlValidatorSanitizers INTERFACE
                ${SIMPLE_XML_VALIDATOR_SANITIZER_FLAGS})
    else ()
        message(WARNING "Sanitizer 选项当前仅支持 GNU 和 Clang 编译器，已忽略。")
    endif ()
endif ()

find_package(XercesC CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)



add_library(SimpleXmlValidatorCore STATIC
        src/core/validation/InputValidator.cpp
        src/core/validation/XmlValidator.cpp
        src/infrastructure/xerces/XercesString.cpp
        src/infrastructure/xerces/XercesRuntime.cpp
        src/infrastructure/xerces/XercesErrorHandler.cpp
        src/infrastructure/xerces/LocalResourceResolver.cpp
        src/infrastructure/xerces/XercesSchemaValidator.cpp
        src/infrastructure/logging/Logger.cpp
        src/infrastructure/logging/ValidationLog.cpp
)
target_include_directories(SimpleXmlValidatorCore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(SimpleXmlValidatorCore PRIVATE
        XercesC::XercesC
        spdlog::spdlog
)
target_link_libraries(SimpleXmlValidatorCore PUBLIC
        SimpleXmlValidatorSanitizers
)

add_executable(SimpleXmlValidator
        main.cpp
        src/gui/MainWindow.cpp
        src/gui/ValidationResultPresenter.cpp
        src/gui/Style.cpp
        src/gui/XmlSyntaxHighlighter.cpp
        src/gui/CodeEditor.cpp
)
target_link_libraries(SimpleXmlValidator
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        SimpleXmlValidatorCore
)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

# Copy Qt DLLs for Windows
if (WIN32)
    get_filename_component(QT_INSTALL_PATH "${Qt5Core_DIR}" DIRECTORY)
    get_filename_component(QT_INSTALL_PATH "${QT_INSTALL_PATH}" DIRECTORY)
    get_filename_component(QT_INSTALL_PATH "${QT_INSTALL_PATH}" DIRECTORY)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${QT_INSTALL_PATH}/plugins/platforms/qwindowsd.dll,${QT_INSTALL_PATH}/plugins/platforms/qwindows.dll>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt5::${QT_LIB}>"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
