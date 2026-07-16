find_package(GTest REQUIRED)

add_executable(SimpleXmlValidatorTests
        core/validation/InputValidatorTest.cpp
        core/validation/XmlValidatorTest.cpp
        infrastructure/logging/LoggerTest.cpp
        infrastructure/xerces/XercesStringTest.cpp
        infrastructure/xerces/XercesErrorHandlerTest.cpp
        infrastructure/xerces/XercesSchemaValidatorTest.cpp
)

target_link_libraries(SimpleXmlValidatorTests PRIVATE
        SimpleXmlValidatorCore
        GTest::gtest_main
)

# 供测试定位 tests/xml 与 tests/xsd 样例，避免依赖 CTest 工作目录。
target_compile_definitions(SimpleXmlValidatorTests PRIVATE
        SIMPLE_XML_VALIDATOR_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

include(GoogleTest)
gtest_discover_tests(SimpleXmlValidatorTests)
