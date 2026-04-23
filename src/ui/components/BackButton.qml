import QtQuick
import QtQuick.Shapes

// Unified back button: 34×34 ghost style with left-arrow icon.
Rectangle {
    id: root
    signal clicked()

    width: 34; height: 34
    radius: 8
    color: mouse.pressed ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.07)
    border.color: Qt.rgba(1, 1, 1, 0.10)
    border.width: 1

    Shape {
        anchors.centerIn: parent
        width: 24; height: 24
        ShapePath {
            fillColor: "#FFFFFF"
            strokeColor: "transparent"
            PathSvg { path: "M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
