import QtQuick

// Dark rounded card container. Place content as children.
Rectangle {
    default property alias content: inner.data

    color: "#1C1C1E"
    radius: 12
    border.color: Qt.rgba(1, 1, 1, 0.06)
    border.width: 1

    Item {
        id: inner
        anchors.fill: parent
    }
}
