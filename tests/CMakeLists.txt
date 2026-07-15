find_package(GTest REQUIRED)

add_executable(SimpleXmlValidatorTests
        core/validation/InputValidatorTest.cpp
        infrastructure/xerces/XercesStringTest.cpp
        infrastructure/xerces/XercesErrorHandlerTest.cpp
)

target_link_libraries(SimpleXmlValidatorTests PRIVATE
        SimpleXmlValidatorCore
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(SimpleXmlValidatorTests)
