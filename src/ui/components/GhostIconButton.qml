import QtQuick
import QtQuick.Shapes

// 34×34 ghost-style icon button with SVG path icon.
// active: blue-tinted selected state
// destructive: red-tinted destructive state
Rectangle {
    id: root

    required property string iconPath

    property bool active: false
    property bool destructive: false

    signal clicked()

    width: 34; height: 34
    radius: 8

    color: {
        if (destructive)
            return mouse.pressed ? Qt.rgba(1, 59/255, 48/255, 0.25) : Qt.rgba(1, 59/255, 48/255, 0.15)
        if (active)
            return Qt.rgba(0, 122/255, 1, 0.20)
        return mouse.pressed ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.07)
    }

    border.color: {
        if (destructive) return Qt.rgba(1, 59/255, 48/255, 0.30)
        if (active)      return Qt.rgba(0, 122/255, 1, 0.40)
        return Qt.rgba(1, 1, 1, 0.10)
    }
    border.width: 1

    Shape {
        anchors.centerIn: parent
        width: 24; height: 24
        ShapePath {
            fillColor: root.destructive ? "#FF3B30" : (root.active ? "#007AFF" : "#EBEBF5")
            strokeColor: "transparent"
            PathSvg { path: root.iconPath }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
