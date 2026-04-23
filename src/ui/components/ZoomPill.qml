import QtQuick
import QtQuick.Shapes

// Zoom pill: [−] [scale%] [+]
// Bind `currentScale` from outside; listen to `zoomRequested(newScale)`.
Rectangle {
    id: root

    required property real currentScale
    property real minScale: 0.1
    property real maxScale: 15.0
    property real step: 0.25

    signal zoomRequested(real newScale)

    height: 34
    width: row.implicitWidth + 16
    radius: 8
    color: Qt.rgba(1, 1, 1, 0.06)
    border.color: Qt.rgba(1, 1, 1, 0.10)
    border.width: 1

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 4

        Rectangle {
            width: 22; height: 22
            radius: 5
            color: zoomOutMouse.pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.08)
            anchors.verticalCenter: parent.verticalCenter

            Shape {
                anchors.centerIn: parent
                width: 24; height: 24
                ShapePath {
                    fillColor: "#EBEBF5"
                    strokeColor: "transparent"
                    PathSvg { path: "M19 13H5v-2h14v2z" }
                }
            }
            MouseArea {
                id: zoomOutMouse
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.zoomRequested(Math.max(root.minScale, root.currentScale - root.step))
            }
        }

        Text {
            width: 44
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 12
            color: "#EBEBF5"
            text: Math.round(root.currentScale * 100) + "%"
        }

        Rectangle {
            width: 22; height: 22
            radius: 5
            color: zoomInMouse.pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.08)
            anchors.verticalCenter: parent.verticalCenter

            Shape {
                anchors.centerIn: parent
                width: 24; height: 24
                ShapePath {
                    fillColor: "#EBEBF5"
                    strokeColor: "transparent"
                    PathSvg { path: "M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" }
                }
            }
            MouseArea {
                id: zoomInMouse
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.zoomRequested(Math.min(root.maxScale, root.currentScale + root.step))
            }
        }
    }
}
