#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_URI_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_URI_H

#include <filesystem>
#include <string>

namespace simple_xml_validator::infrastructure::xerces {

// 将文件系统路径转为规范的 file:// URI（正斜杠 + percent-encoding）。
// setExternalSchemaLocation 等 Xerces API 要求 URI 而非原始路径；
// 原始非 ASCII 路径在 Windows/macOS 上会导致 schema 绑定失败。
std::string toFileUri(const std::filesystem::path& path);

}

#endif
