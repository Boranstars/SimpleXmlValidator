#ifndef SIMPLE_XML_VALIDATOR_TESTS_INFRASTRUCTURE_XERCES_XERCES_TEST_FIXTURE_H
#define SIMPLE_XML_VALIDATOR_TESTS_INFRASTRUCTURE_XERCES_XERCES_TEST_FIXTURE_H

#include "infrastructure/xerces/XercesRuntime.h"

#include <gtest/gtest.h>

#include <memory>

class XercesTestFixture : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        runtime_ = std::make_unique<simple_xml_validator::infrastructure::xerces::XercesRuntime>();
    }

    static void TearDownTestSuite() {
        runtime_.reset();
    }

    void SetUp() override {
        if (!runtime_ || !runtime_->isReady()) {
            GTEST_SKIP() << "Xerces 初始化失败："
                         << (runtime_ ? runtime_->errorMessage() : "运行时未创建");
        }
    }

    static const simple_xml_validator::infrastructure::xerces::XercesRuntime& runtime() {
        return *runtime_;
    }

private:
    inline static std::unique_ptr<simple_xml_validator::infrastructure::xerces::XercesRuntime> runtime_;
};

#endif
