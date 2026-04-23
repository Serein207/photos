import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Menu {
    id: contextMenu
    width: 160
    topPadding: 6
    bottomPadding: 6

    required property var viewModel
    required property string sourceUrl
    required property string selectedOcrText
    required property bool isEditing

    signal subjectHoverChanged(bool hovered)

    background: Item {
        implicitWidth: 160
        RectangularShadow {
            anchors.fill: menuBg
            radius: 10
            blur: 28
            spread: 2
            offset: Qt.vector2d(0, 6)
            color: Qt.rgba(0, 0, 0, 0.55)
        }
        Rectangle {
            id: menuBg
            anchors.fill: parent
            color: Qt.rgba(30/255, 30/255, 32/255, 0.96)
            radius: 10
            border.color: Qt.rgba(1, 1, 1, 0.12)
            border.width: 1
        }
    }

    component StyledMenuItem: MenuItem {
        id: menuItem
        height: visible ? implicitHeight : 0
        implicitHeight: 34
        contentItem: Text {
            text: menuItem.text
            font.pixelSize: 13
            color: menuItem.enabled ? "#EBEBF5" : "#48484A"
            verticalAlignment: Text.AlignVCenter
            leftPadding: 12
        }
        background: Rectangle {
            color: menuItem.highlighted ? Qt.rgba(1, 1, 1, 0.10) : "transparent"
            radius: 6
            x: 4; y: 2
            width: parent.width - 8
            height: parent.height - 4
        }
    }

    StyledMenuItem {
        text: "Copy Image"
        onTriggered: contextMenu.viewModel.copyImageToClipboard(contextMenu.sourceUrl)
    }
    StyledMenuItem {
        text: "Copy Subject"
        hoverEnabled: true
        enabled: !contextMenu.isEditing
        onHoveredChanged: contextMenu.subjectHoverChanged(hovered)
        onTriggered: contextMenu.viewModel.extractSubject(contextMenu.sourceUrl)
    }
    MenuSeparator {
        visible: contextMenu.selectedOcrText !== ""
        contentItem: Rectangle {
            implicitHeight: 1
            color: Qt.rgba(1, 1, 1, 0.08)
        }
    }
    StyledMenuItem {
        text: "Copy Text"
        visible: contextMenu.selectedOcrText !== ""
        enabled: !contextMenu.isEditing
        onTriggered: contextMenu.viewModel.copyTextToClipboard(contextMenu.selectedOcrText)
    }
}
