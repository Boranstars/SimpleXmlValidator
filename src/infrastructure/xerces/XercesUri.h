#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_URI_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_URI_H

#include <filesystem>
#include <string>

namespace simple_xml_validator::infrastructure::xerces {

// 将文件系统路径转为规范的 file:// URI（正斜杠 + percent-encoding）。
std::string toFileUri(const std::filesystem::path& path);

// 将 file:// URI 还原为文件系统路径（percent-decode + 平台路径格式）。
// 若输入不以 "file://" 开头，返回空路径。
std::filesystem::path pathFromFileUri(std::string_view fileUri);

}

#endif
