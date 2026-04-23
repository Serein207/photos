pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import photos 1.0

Item {
    id: galleryView
    signal photoClicked(string source)
    signal settingsClicked

    PhotoViewModel {
        id: photoViewModel
    }

    Rectangle {
        id: topBar
        width: parent.width
        height: 52
        color: Qt.rgba(28/255, 28/255, 30/255, 0.88)
        z: 20

        Text {
            anchors.centerIn: parent
            text: "Gallery"
            font.pixelSize: 20
            font.bold: true
            color: "#FFFFFF"
        }

        Item {
            width: 44
            height: parent.height
            anchors.right: parent.right

            Text {
                anchors.centerIn: parent
                text: "\u2699"
                font.pixelSize: 22
                color: "#FFFFFF"
                opacity: gearHover.containsMouse ? 1.0 : 0.55
                Behavior on opacity { NumberAnimation { duration: 120 } }
            }

            MouseArea {
                id: gearHover
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onClicked: galleryView.settingsClicked()
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Qt.rgba(1, 1, 1, 0.08)
        }
    }

    RectangularShadow {
        anchors.left: topBar.left
        anchors.right: topBar.right
        anchors.top: topBar.bottom
        height: 1
        radius: 0
        blur: 16
        spread: 0
        offset: Qt.vector2d(0, 0)
        color: Qt.rgba(0, 0, 0, 0.50)
        z: 19
    }

    Rectangle {
        anchors.fill: parent
        color: "#111111"
        z: 0
    }

    GridView {
        id: grid
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 20
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        cellWidth: 232
        cellHeight: 232
        model: photoViewModel.gallery.galleryImages
        clip: true
        z: 10

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 6
                radius: 3
                color: Qt.rgba(1, 1, 1, 0.35)
            }
            background: Rectangle { color: "transparent" }
        }

        delegate: Item {
            id: delegateItem
            required property string modelData
            width: grid.cellWidth
            height: grid.cellHeight

            Item {
                id: cardContainer
                anchors.fill: parent
                anchors.margins: 10

                RectangularShadow {
                    anchors.fill: cardBg
                    radius: 10
                    blur: 20
                    spread: 0
                    offset: Qt.vector2d(0, 4)
                    color: Qt.rgba(0, 0, 0, 0.45)
                    cached: true
                }

                Rectangle {
                    id: cardBg
                    anchors.fill: parent
                    color: "#1C1C1E"
                    radius: 10
                    border.color: Qt.rgba(1, 1, 1, 0.06)
                    border.width: 1
                }

                Item {
                    anchors.fill: parent

                    Rectangle {
                        id: imageMask
                        anchors.fill: parent
                        color: "black"
                        radius: 10
                        visible: false
                        layer.enabled: true
                    }

                    Image {
                        id: cardImg
                        anchors.fill: parent
                        source: delegateItem.modelData
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            maskEnabled: true
                            maskSource: imageMask
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: {
                        cardBg.color = "#2C2C2E"
                    }
                    onExited: {
                        cardBg.color = "#1C1C1E"
                    }
                    onClicked: {
                        galleryView.photoClicked(delegateItem.modelData)
                    }
                }
            }
        }
    }
}
