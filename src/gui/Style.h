#ifndef SIMPLE_XML_VALIDATOR_GUI_STYLE_H
#define SIMPLE_XML_VALIDATOR_GUI_STYLE_H

#include <QString>

namespace simple_xml_validator::gui {

// 返回全局应用样式表（纯 QSS，不依赖任何图片/资源文件）。
// 由组合根在创建 QApplication 后统一应用，使界面接近 docs/ui_raw/概念设计图.jpg。
QString applicationStyleSheet();

}  // namespace simple_xml_validator::gui

#endif
