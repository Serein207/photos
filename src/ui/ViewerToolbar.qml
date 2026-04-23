import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: viewerToolbar
    height: 56
    z: 20

    required property bool isGif
    required property bool isEditorVisible
    required property real currentScale
    required property bool isOcrLoading

    signal backClicked()
    signal rotateClicked()
    signal editClicked()
    signal zoomRequested(real newScale)

    // Dark frosted background
    Rectangle {
        id: topBarBg
        anchors.fill: parent
        color: Qt.rgba(28/255, 28/255, 30/255, 0.88)
    }

    RectangularShadow {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.bottom
        height: 1
        radius: 0
        blur: 18
        spread: 0
        offset: Qt.vector2d(0, 0)
        color: Qt.rgba(0, 0, 0, 0.55)
        z: -1
    }

    // Bottom border
    Divider {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        z: 1
    }

    // Back button
    BackButton {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 12
        onClicked: viewerToolbar.backClicked()
    }

    // Center group: zoom pill + rotate + OCR spinner
    Row {
        id: centerRow
        anchors.centerIn: parent
        spacing: 8
        visible: !viewerToolbar.isGif && !viewerToolbar.isEditorVisible

        // Zoom pill
        ZoomPill {
            currentScale: viewerToolbar.currentScale
            anchors.verticalCenter: parent.verticalCenter
            onZoomRequested: (s) => viewerToolbar.zoomRequested(s)
        }

        // Rotate button
        GhostIconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconPath: "M15.55 5.55L11 1v3C7.13 4 4 7.13 4 11s3.13 7 7 7c1.53 0 2.96-.49 4.13-1.33l-1.45-1.45C12.91 15.7 12 16 11 16c-2.76 0-5-2.24-5-5s2.24-5 5-5v3l4.55-4.55z"
            onClicked: viewerToolbar.rotateClicked()
        }

        // OCR spinner — visible only when OCR is running
        Item {
            id: ocrSpinner
            width: 22; height: 22
            visible: viewerToolbar.isOcrLoading
            anchors.verticalCenter: parent.verticalCenter

            Canvas {
                anchors.fill: parent
                onVisibleChanged: if (visible) requestPaint()
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.beginPath()
                    ctx.arc(11, 11, 8, 0, Math.PI * 2)
                    ctx.strokeStyle = "rgba(255,255,255,0.12)"
                    ctx.lineWidth = 2.5
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.arc(11, 11, 8, -Math.PI / 2, 0)
                    ctx.strokeStyle = "#007AFF"
                    ctx.lineWidth = 2.5
                    ctx.lineCap = "round"
                    ctx.stroke()
                }
            }

            RotationAnimator {
                target: ocrSpinner
                from: 0; to: 360
                duration: 800
                loops: Animation.Infinite
                running: ocrSpinner.visible
            }
        }
    }

    // Edit button — accent style
    Rectangle {
        id: editBtnBg
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 12
        width: 34; height: 34
        radius: 8
        color: editMouse.pressed ? "#0066DD" : "#007AFF"
        visible: !viewerToolbar.isGif && !viewerToolbar.isEditorVisible

        Shape {
            anchors.centerIn: parent
            width: 24; height: 24
            ShapePath {
                fillColor: "#FFFFFF"
                strokeColor: "transparent"
                PathSvg { path: "M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" }
            }
        }
        MouseArea {
            id: editMouse
            anchors.fill: parent
            onClicked: viewerToolbar.editClicked()
        }
    }
}
