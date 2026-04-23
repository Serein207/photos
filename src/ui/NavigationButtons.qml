import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Item {
    id: navButtons

    required property bool hasPrevious
    required property bool hasNext

    signal previousClicked()
    signal nextClicked()

    Button {
        id: prevBtn
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 16
        width: 40; height: 40
        visible: navButtons.hasPrevious
        opacity: prevHover.hovered ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 250 } }
        HoverHandler { id: prevHover; margin: 40 }
        background: Rectangle { radius: width / 2; color: Qt.rgba(0, 0, 0, 0.5) }
        contentItem: Item {
            width: 24; height: 24
            Shape {
                anchors.centerIn: parent
                width: 24; height: 24
                ShapePath {
                    fillColor: "#FFFFFF"
                    strokeColor: "transparent"
                    PathSvg { path: "M15.41 16.59L10.83 12l4.58-4.59L14 6l-6 6 6 6 1.41-1.41z" }
                }
            }
        }
        onClicked: navButtons.previousClicked()
    }

    Button {
        id: nextBtn
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 16
        width: 40; height: 40
        visible: navButtons.hasNext
        opacity: nextHover.hovered ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 250 } }
        HoverHandler { id: nextHover; margin: 40 }
        background: Rectangle { radius: width / 2; color: Qt.rgba(0, 0, 0, 0.5) }
        contentItem: Item {
            width: 24; height: 24
            Shape {
                anchors.centerIn: parent
                width: 24; height: 24
                ShapePath {
                    fillColor: "#FFFFFF"
                    strokeColor: "transparent"
                    PathSvg { path: "M8.59 16.59L13.17 12 8.59 7.41 10 6l6 6-6 6-1.41-1.41z" }
                }
            }
        }
        onClicked: navButtons.nextClicked()
    }
}
