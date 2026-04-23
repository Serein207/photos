pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import "ui" as UI

Window {
    id: window
    width: 800
    height: 600
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: qsTr("Photos")
    color: "#111111"

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: UI.GalleryView {
            onPhotoClicked: (source) => {
                stackView.push(photoViewerComponent, {sourceUrl: source})
            }
            onSettingsClicked: {
                stackView.push(settingsPageComponent)
            }
        }
    }

    Component {
        id: photoViewerComponent
        UI.PhotoViewer {
            onBackClicked: stackView.pop()
        }
    }

    Component {
        id: settingsPageComponent
        UI.SettingsPage {
            onBackClicked: stackView.pop()
        }
    }
}
