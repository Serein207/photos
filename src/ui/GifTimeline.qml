pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: root
    width: 600
    height: 120

    property AnimatedImage targetImage: null
    property bool isPlaying: true
    property real playbackSpeed: 1.0

    onIsPlayingChanged: {
        if (targetImage) {
            targetImage.paused = !isPlaying
        }
    }

    onPlaybackSpeedChanged: {
        if (targetImage) targetImage.speed = playbackSpeed
    }

    onTargetImageChanged: {
        if (targetImage) targetImage.speed = playbackSpeed
    }

    signal frameSeeked(int frameIndex)

    Item {
        anchors.fill: parent

        RectangularShadow {
            anchors.fill: timelineBg
            radius: timelineBg.radius
            blur: 24
            spread: 0
            offset: Qt.vector2d(0, 6)
            color: Qt.rgba(0, 0, 0, 0.5)
            cached: true
        }

        Rectangle {
            id: timelineBg
            anchors.fill: parent
            color: "#1C1C1E"
            radius: 12
            border.color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 5

            ListView {
                id: thumbnailList
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                orientation: ListView.Horizontal
                spacing: 5
                clip: true

                property int prevFrame: 0
                highlightMoveDuration: (currentIndex === 0 && prevFrame > 0) ? 0 : 250
                onCurrentIndexChanged: prevFrame = currentIndex

                currentIndex: root.targetImage ? root.targetImage.currentFrame : 0
                preferredHighlightBegin: Math.max(0, width / 2 - 30)
                preferredHighlightEnd: Math.max(0, width / 2 + 30)
                highlightRangeMode: ListView.StrictlyEnforceRange

                header: Item { width: Math.max(0, thumbnailList.width / 2 - 30); height: 60 }
                footer: Item { width: Math.max(0, thumbnailList.width / 2 - 30); height: 60 }

                model: root.targetImage ? root.targetImage.frameCount : 0

                delegate: Rectangle {
                    id: frameDelegate
                    required property int index
                    width: 60
                    height: 60
                    color: "transparent"
                    radius: 4
                    border.color: frameDelegate.index === (root.targetImage ? root.targetImage.currentFrame : -1) ? "#007AFF" : Qt.rgba(1, 1, 1, 0.15)
                    border.width: frameDelegate.index === (root.targetImage ? root.targetImage.currentFrame : -1) ? 2 : 1

                    AnimatedImage {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: root.targetImage ? root.targetImage.source : ""
                        currentFrame: frameDelegate.index
                        playing: false
                        fillMode: Image.PreserveAspectCrop
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.isPlaying = false
                            if (root.targetImage) {
                                root.targetImage.currentFrame = frameDelegate.index
                                root.frameSeeked(frameDelegate.index)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                // Play/Pause button
                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: 18
                    color: playPauseMa.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : "transparent"

                    Shape {
                        anchors.centerIn: parent
                        width: 20; height: 20
                        ShapePath {
                            fillColor: "#EBEBF5"
                            strokeColor: "transparent"
                            PathSvg {
                                path: root.isPlaying
                                    ? "M5 17h4V5H5v12zm8-12v12h4V5h-4z"
                                    : "M6 4.5v13l10-6.5z"
                            }
                        }
                    }

                    MouseArea {
                        id: playPauseMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.isPlaying = !root.isPlaying
                    }
                }

                // Timeline scrubber track
                Item {
                    Layout.fillWidth: true
                    height: 36

                    Rectangle {
                        id: trackBg
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 4
                        radius: 2
                        color: Qt.rgba(1, 1, 1, 0.15)
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: trackBg.left
                        width: root.targetImage && root.targetImage.frameCount > 1
                            ? trackBg.width * root.targetImage.currentFrame / (root.targetImage.frameCount - 1)
                            : 0
                        height: 4
                        radius: 2
                        color: "#007AFF"
                    }

                    Rectangle {
                        id: scrubHandle
                        anchors.verticalCenter: parent.verticalCenter
                        x: root.targetImage && root.targetImage.frameCount > 1
                            ? trackBg.width * root.targetImage.currentFrame / (root.targetImage.frameCount - 1) - 7
                            : -7
                        width: 14; height: 14
                        radius: 7
                        color: "#FFFFFF"

                        MouseArea {
                            anchors.fill: parent
                            drag.target: parent
                            drag.axis: Drag.XAxis
                            drag.minimumX: -7
                            drag.maximumX: trackBg.width - 7
                            cursorShape: Qt.SizeHorCursor
                            onPressed: root.isPlaying = false
                            onPositionChanged: {
                                if (pressed && root.targetImage && root.targetImage.frameCount > 1) {
                                    const t = Math.max(0, Math.min(1, (scrubHandle.x + 7) / trackBg.width));
                                    const frame = Math.round(t * (root.targetImage.frameCount - 1));
                                    root.targetImage.currentFrame = frame;
                                    root.frameSeeked(frame);
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onPressed: (mouse) => {
                            root.isPlaying = false;
                            if (root.targetImage && root.targetImage.frameCount > 1) {
                                const t = Math.max(0, Math.min(1, mouse.x / trackBg.width));
                                const frame = Math.round(t * (root.targetImage.frameCount - 1));
                                root.targetImage.currentFrame = frame;
                                root.frameSeeked(frame);
                            }
                        }
                    }
                }

                Text {
                    text: (root.targetImage ? root.targetImage.currentFrame : 0) + " / " + (root.targetImage ? Math.max(0, root.targetImage.frameCount - 1) : 0)
                    color: Qt.rgba(1, 1, 1, 0.55)
                    font.pixelSize: 12
                    Layout.preferredWidth: 55
                    horizontalAlignment: Text.AlignRight
                }

                // Speed selector
                Row {
                    spacing: 3
                    Repeater {
                        model: [0.25, 0.5, 1.0, 2.0, 4.0]
                        delegate: Rectangle {
                            required property var modelData
                            width: modelData < 1.0 ? 46 : 36; height: 22
                            radius: 4
                            color: Math.abs(root.playbackSpeed - modelData) < 0.01
                                ? "#007AFF"
                                : (speedMa.containsMouse ? Qt.rgba(1,1,1,0.12) : Qt.rgba(1,1,1,0.06))

                            Text {
                                anchors.centerIn: parent
                                text: modelData === 0.25 ? "0.25×"
                                    : modelData === 0.5  ? "0.5×"
                                    : modelData === 1.0  ? "1×"
                                    : modelData === 2.0  ? "2×"
                                    :                      "4×"
                                font.pixelSize: 11
                                color: Math.abs(root.playbackSpeed - modelData) < 0.01
                                    ? "#FFFFFF"
                                    : Qt.rgba(1, 1, 1, 0.7)
                            }

                            MouseArea {
                                id: speedMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.playbackSpeed = modelData
                            }
                        }
                    }
                }
            }
        }
    }
}
