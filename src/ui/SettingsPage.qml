pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import photos 1.0

Item {
    id: settingsPage
    signal backClicked

    PhotoViewModel {
        id: vm
    }

    Component.onCompleted: refreshStats()
    onVisibleChanged: if (visible) refreshStats()

    function refreshStats() {
        entriesLabel.text = vm.cacheEntries() + " items"
        var bytes = vm.cacheSizeBytes()
        if (bytes < 1024)
            sizeLabel.text = bytes + " B"
        else if (bytes < 1024 * 1024)
            sizeLabel.text = (bytes / 1024).toFixed(1) + " KB"
        else
            sizeLabel.text = (bytes / (1024 * 1024)).toFixed(2) + " MB"
        maxField.text = vm.cacheMaxEntries()
    }

    Rectangle {
        anchors.fill: parent
        color: "#111111"
    }

    Rectangle {
        id: topBar
        width: parent.width
        height: 52
        color: Qt.rgba(28/255, 28/255, 30/255, 0.88)
        z: 10

        BackButton {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            onClicked: settingsPage.backClicked()
        }

        Text {
            anchors.centerIn: parent
            text: "Settings"
            font.pixelSize: 20
            font.bold: true
            color: "#FFFFFF"
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Qt.rgba(1, 1, 1, 0.08)
        }
    }

    Column {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 24
        anchors.topMargin: 32
        spacing: 0

        Text {
            text: "OCR CACHE"
            font.pixelSize: 13
            font.bold: true
            color: Qt.rgba(1, 1, 1, 0.45)
            leftPadding: 4
            bottomPadding: 8
        }

        CardContainer {
            width: parent.width
            height: cardCol.implicitHeight + 8

            Column {
                id: cardCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 4

                Item {
                    width: parent.width
                    height: 48
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Cached items"; font.pixelSize: 16; color: "#FFFFFF"
                    }
                    Text {
                        id: entriesLabel
                        anchors.right: parent.right; anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "—"; font.pixelSize: 16; color: Qt.rgba(1, 1, 1, 0.45)
                    }
                }

                Divider { separatorOpacity: 0.06 }

                Item {
                    width: parent.width
                    height: 48
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Cache size"; font.pixelSize: 16; color: "#FFFFFF"
                    }
                    Text {
                        id: sizeLabel
                        anchors.right: parent.right; anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "—"; font.pixelSize: 16; color: Qt.rgba(1, 1, 1, 0.45)
                    }
                }

                Divider { separatorOpacity: 0.06 }

                Item {
                    width: parent.width
                    height: 56
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Max entries"; font.pixelSize: 16; color: "#FFFFFF"
                    }
                    Rectangle {
                        anchors.right: parent.right; anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        width: 72; height: 34; radius: 8
                        color: Qt.rgba(1, 1, 1, 0.08)
                        border.color: maxField.activeFocus ? "#0A84FF" : Qt.rgba(1,1,1,0.12)
                        border.width: 1

                        TextInput {
                            id: maxField
                            anchors.fill: parent; anchors.margins: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignHCenter
                            font.pixelSize: 15; color: "#FFFFFF"
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator { bottom: 1; top: 9999 }
                            text: "100"
                            onEditingFinished: {
                                var n = parseInt(text)
                                if (n > 0) {
                                    vm.setCacheMaxEntries(n)
                                    settingsPage.refreshStats()
                                }
                            }
                        }
                    }
                }

                Divider { separatorOpacity: 0.06 }

                Item {
                    width: parent.width
                    height: 52
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Clear cache"; font.pixelSize: 16; color: "#FF453A"
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            vm.clearCache()
                            settingsPage.refreshStats()
                        }
                    }
                }
            }
        }
    }
}
