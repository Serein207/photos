import QtQuick

// 1px horizontal separator. Set separatorOpacity to 0.06 for card rows, 0.08 for bar borders.
Rectangle {
    property real separatorOpacity: 0.08

    width: parent ? parent.width : 0
    height: 1
    color: Qt.rgba(1, 1, 1, separatorOpacity)
}
