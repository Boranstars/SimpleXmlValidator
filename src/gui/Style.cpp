#include "gui/Style.h"

#include <QFile>
#include <QTextStream>

namespace simple_xml_validator::gui {

QString applicationStyleSheet() {
    QFile file(QStringLiteral(":/style.qss"));
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return {};
    QTextStream stream(&file);
    return stream.readAll();
}

}  // namespace simple_xml_validator::gui
