import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Item {
    id: cropActionBar

    required property bool cropPending
    required property string activeTool
    required property real cropAspectRatio

    signal confirmCrop()
    signal cancelCrop()
    signal aspectRatioRequested(real ratio)

    visible: activeTool === "crop"

    Rectangle {
        id: barBg
        anchors.fill: parent
        color: Qt.rgba(28/255, 28/255, 30/255, 0.92)
    }

    RectangularShadow {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.top
        height: 1
        radius: 0
        blur: 18
        spread: 0
        offset: Qt.vector2d(0, 0)
        color: Qt.rgba(0, 0, 0, 0.55)
        z: -1
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Qt.rgba(1, 1, 1, 0.08)
        z: 1
    }

    Row {
        anchors.centerIn: parent
        spacing: 10

        component RatioPill: Rectangle {
            id: pill
            required property string label
            required property real ratio
            property bool isActive: Math.abs(cropActionBar.cropAspectRatio - ratio) < 0.01

            height: 34
            width: pillText.implicitWidth + 24
            radius: 8
            color: isActive ? Qt.rgba(0, 122/255, 1, 0.20) : Qt.rgba(1, 1, 1, 0.07)
            border.color: isActive ? Qt.rgba(0, 122/255, 1, 0.40) : Qt.rgba(1, 1, 1, 0.10)
            border.width: 1

            Text {
                id: pillText
                anchors.centerIn: parent
                text: pill.label
                font.pixelSize: 12
                color: pill.isActive ? "#007AFF" : "#8E8E93"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: cropActionBar.aspectRatioRequested(pill.ratio)
            }
        }

        RatioPill { label: "Free"; ratio: -1 }
        RatioPill { label: "1:1";  ratio: 1.0 }
        RatioPill { label: "4:3";  ratio: 4.0 / 3.0 }
        RatioPill { label: "16:9"; ratio: 16.0 / 9.0 }

        Rectangle {
            width: 1; height: 24
            color: Qt.rgba(1, 1, 1, 0.10)
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {
            id: cancelBtn
            height: 34
            width: cancelText.implicitWidth + 24
            radius: 8
            color: cancelMouse.pressed ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.07)
            border.color: Qt.rgba(1, 1, 1, 0.10)
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: cancelText
                anchors.centerIn: parent
                text: "Cancel"
                font.pixelSize: 13
                font.weight: Font.Medium
                color: "#EBEBF5"
            }
            MouseArea {
                id: cancelMouse
                anchors.fill: parent
                onClicked: cropActionBar.cancelCrop()
            }
        }

        Rectangle {
            id: applyBtn
            height: 34
            width: applyText.implicitWidth + 24
            radius: 8
            color: applyMouse.pressed ? "#0066DD" : "#007AFF"
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: applyText
                anchors.centerIn: parent
                text: "Apply"
                font.pixelSize: 13
                font.weight: Font.Medium
                color: "#FFFFFF"
            }
            MouseArea {
                id: applyMouse
                anchors.fill: parent
                onClicked: cropActionBar.confirmCrop()
            }
        }
    }
}
