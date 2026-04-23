import QtQuick
import QtQuick.Controls
import photos 1.0

Item {
    id: photoViewer
    focus: true

    Shortcut {
        sequences: [ StandardKey.Copy ]
        enabled: photoViewer.visible
        onActivated: {
            if (liveTextOverlay.selectedText.length > 0)
                photoViewModel.copyTextToClipboard(liveTextOverlay.selectedText)
            else
                photoViewModel.copyImageToClipboard(photoViewer.sourceUrl)
        }
    }

    Keys.onPressed: (event) => {
        if (photoViewer.isEditing) {
            if (event.key === Qt.Key_Escape) {
                editor.visible = false;
                event.accepted = true;
            }
            return;
        }
        if ((event.key === Qt.Key_C) && (event.modifiers & Qt.ControlModifier)) {
            event.accepted = true
        } else if (event.key === Qt.Key_Left) {
            if (photoViewer.hasPrevious) {
                photoViewer.navDirection = -1
                photoViewer.zoomToCenter(1.0, false)
                photoViewer.sourceUrl = photoViewModel.gallery.galleryImages[photoViewer.currentIndex - 1]
            }
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            if (photoViewer.hasNext) {
                photoViewer.navDirection = 1
                photoViewer.zoomToCenter(1.0, false)
                photoViewer.sourceUrl = photoViewModel.gallery.galleryImages[photoViewer.currentIndex + 1]
            }
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            photoViewer.backClicked()
            event.accepted = true
        }
    }
    property string sourceUrl: ""
    property string baseSourceUrl: ""
    // -1 = going left (prev), 1 = going right (next), 0 = no direction
    property int navDirection: 0
    property bool isGif: sourceUrl.toLowerCase().endsWith(".gif")
    property int currentIndex: photoViewModel.gallery.galleryImages.indexOf(baseSourceUrl)
    onCurrentIndexChanged: {
        if (currentIndex === -1 && baseSourceUrl !== "") {
            Qt.callLater(() => {
                if (photoViewer.currentIndex === -1) {
                    if (photoViewModel.gallery.galleryImages.length > 0) {
                        photoViewer.sourceUrl = photoViewModel.gallery.galleryImages[0]
                    } else {
                        photoViewer.backClicked()
                    }
                }
            })
        }
    }
    property bool hasPrevious: currentIndex > 0
    property bool hasNext: currentIndex >= 0 && currentIndex < photoViewModel.gallery.galleryImages.length - 1
    property string selectedOcrText: ""
    property bool subjectHovered: false
    property bool isEditing: editor.visible
    property int viewRotation: 0
    property bool suppressRotationAnim: false
    signal backClicked()
    property alias currentScale: imageContainer.scale

    property size activeSourceSize: isGif ? animatedImage.sourceSize : image.sourceSize

    function calculateFitScale() {
        if (activeSourceSize.width === 0 || activeSourceSize.height === 0) return 1.0;
        const isRotated = (photoViewer.viewRotation % 180 !== 0);
        const imgW = isRotated ? activeSourceSize.height : activeSourceSize.width;
        const imgH = isRotated ? activeSourceSize.width : activeSourceSize.height;
        if (imgW === 0 || imgH === 0) return 1.0;

        const fw = Math.max(1, flickable.width);
        const fh = Math.max(1, flickable.height);
        const scaleX = fw / imgW;
        const scaleY = fh / imgH;
        const scale = Math.min(scaleX, scaleY);
        return Math.min(scale, 1.0);
    }

    function resetRotation() {
        photoViewer.suppressRotationAnim = true
        photoViewer.viewRotation = 0
        photoViewer.suppressRotationAnim = false
    }

    function zoomToPoint(newScale, pointX, pointY, animated) {
        if (animated === undefined) animated = false;
        const s1 = imageContainer.scale;
        const s2 = Math.max(0.05, Math.min(newScale, 15.0));
        if (Math.abs(s1 - s2) < 0.001) return;

        zoomAnimGroup.stop();

        const W = imageContainer.width;
        const H = imageContainer.height;

        const imgTargetX = (flickable.contentX + pointX - imageContainer.x) / s1;
        const imgTargetY = (flickable.contentY + pointY - imageContainer.y) / s1;

        const newImgX = Math.max(0, (flickable.width - W * s2) / 2);
        const newImgY = Math.max(0, (flickable.height - H * s2) / 2);

        let newContentX = newImgX + imgTargetX * s2 - pointX;
        let newContentY = newImgY + imgTargetY * s2 - pointY;

        const CW2 = Math.max(flickable.width, W * s2);
        const CH2 = Math.max(flickable.height, H * s2);

        newContentX = Math.max(0, Math.min(newContentX, CW2 - flickable.width));
        newContentY = Math.max(0, Math.min(newContentY, CH2 - flickable.height));

        if (animated) {
            scaleAnim.to = s2;
            scrollXAnim.to = newContentX;
            scrollYAnim.to = newContentY;
            zoomAnimGroup.restart();
        } else {
            imageContainer.scale = s2;
            flickable.contentX = newContentX;
            flickable.contentY = newContentY;
        }
    }

    function zoomToCenter(newScale, animated) {
        if (animated === undefined) animated = true;
        zoomToPoint(newScale, flickable.width / 2.0, flickable.height / 2.0, animated);
    }

    ParallelAnimation {
        id: zoomAnimGroup
        NumberAnimation {
            id: scaleAnim
            target: imageContainer
            property: "scale"
            duration: 200
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            id: scrollXAnim
            target: flickable
            property: "contentX"
            duration: 200
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            id: scrollYAnim
            target: flickable
            property: "contentY"
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    
    onSourceUrlChanged: {
        const isTempEdit = sourceUrl.indexOf("/edit_") !== -1;
        if (!isTempEdit) {
            baseSourceUrl = sourceUrl;
        }
        photoViewer.resetRotation();
        extractedSubjectImage.source = "";
        photoViewModel.runOcr("");
        if (sourceUrl !== "" && !isGif) {
            if (!isEditing) photoViewModel.runOcr(sourceUrl);
        }
    }
    
    onIsEditingChanged: {
        if (!isEditing) {
            if (sourceUrl !== "" && !isGif) {
                photoViewModel.runOcr(sourceUrl);
            } else {
                photoViewModel.runOcr("");
            }
        }
    }

    PhotoViewModel {
        id: photoViewModel
        onSubjectExtracted: (subjectUrl) => {
            extractedSubjectImage.source = subjectUrl
            subjectAnimation.restart()
            subjectOpacityAnim.restart()
        }
    }

    ImageContextMenu {
        id: contextMenu
        viewModel: photoViewModel
        sourceUrl: photoViewer.sourceUrl
        selectedOcrText: photoViewer.selectedOcrText
        isEditing: photoViewer.isEditing
        onSubjectHoverChanged: (hovered) => { photoViewer.subjectHovered = hovered }
    }

    function getBoundingRect(poly) {
        if (!poly || poly.length === 0) return {x:0, y:0, width:0, height:0};
        let minX = poly[0].x, maxX = poly[0].x, minY = poly[0].y, maxY = poly[0].y;
        for (let i = 1; i < poly.length; i++) {
            minX = Math.min(minX, poly[i].x);
            maxX = Math.max(maxX, poly[i].x);
            minY = Math.min(minY, poly[i].y);
            maxY = Math.max(maxY, poly[i].y);
        }
        return {x: minX, y: minY, width: maxX - minX, height: maxY - minY};
    }

    Rectangle {
        anchors.fill: parent
        color: "#0D0D0D"

        Flickable {
            id: flickable
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: gifTimeline.visible ? gifTimeline.top : parent.bottom
            anchors.bottomMargin: gifTimeline.visible ? 20 : 0
            anchors.top: topBar.visible ? topBar.bottom : parent.top
            contentWidth: Math.max(width, imageContainer.width * imageContainer.scale)
            contentHeight: Math.max(height, imageContainer.height * imageContainer.scale)
            clip: true
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

            Item {
                id: imageContainer
                x: Math.max(0, (flickable.width - width * scale) / 2)
                y: Math.max(0, (flickable.height - height * scale) / 2)
                width: photoViewer.activeSourceSize.width > 0 ? photoViewer.activeSourceSize.width : flickable.width
                height: photoViewer.activeSourceSize.height > 0 ? photoViewer.activeSourceSize.height : flickable.height
                transformOrigin: Item.TopLeft

                // Slide-in animation when navigating between images
                property real slideOffset: 0
                transform: Translate { x: imageContainer.slideOffset }

                NumberAnimation {
                    id: slideAnim
                    target: imageContainer
                    property: "slideOffset"
                    duration: 220
                    easing.type: Easing.OutCubic
                }

                Connections {
                    target: photoViewer
                    function onSourceUrlChanged() {
                        if (photoViewer.navDirection === 0) return
                        imageContainer.opacity = 0
                        imageContainer.slideOffset = photoViewer.navDirection * 60
                        slideAnim.from = photoViewer.navDirection * 60
                        slideAnim.to = 0
                        slideAnim.restart()
                        fadeInAnim.restart()
                        photoViewer.navDirection = 0
                    }
                }

                NumberAnimation {
                    id: fadeInAnim
                    target: imageContainer
                    property: "opacity"
                    from: 0; to: 1
                    duration: 180
                    easing.type: Easing.OutQuad
                }
                
                Item {
                    id: rotateContainer
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    width: (photoViewer.viewRotation % 180 === 0) ? parent.width : parent.height
                    height: (photoViewer.viewRotation % 180 === 0) ? parent.height : parent.width
                    rotation: photoViewer.viewRotation

                    Behavior on rotation { enabled: !photoViewer.suppressRotationAnim; RotationAnimation { direction: RotationAnimation.Clockwise; duration: 250; easing.type: Easing.InOutQuad } }

                    AnimatedImage {
                        id: animatedImage
                        source: photoViewer.isGif ? photoViewer.sourceUrl.replace("image://photo_provider/", "file://") : ""
                        visible: photoViewer.isGif
                        playing: true // Autoplay, we control pause state via Timeline
                        fillMode: Image.PreserveAspectFit
                        anchors.fill: parent
                        
                        onStatusChanged: {
                            if (status === AnimatedImage.Ready) {
                                gifTimeline.isPlaying = true;
                                if (!photoViewer.isEditing) {
                                    Qt.callLater(function() {
                                        imageContainer.scale = photoViewer.calculateFitScale();
                                    });
                                }
                            }
                        }
                        onSourceSizeChanged: {
                            if (status === AnimatedImage.Ready) {
                                if (!photoViewer.isEditing) {
                                    Qt.callLater(function() {
                                        imageContainer.scale = photoViewer.calculateFitScale();
                                    });
                                }
                            }
                        }
                    }

                    Image {
                        id: image
                        source: !photoViewer.isGif ? photoViewer.sourceUrl : ""
                        visible: !photoViewer.isGif
                        fillMode: Image.PreserveAspectFit
                        anchors.fill: parent
                        
                        onStatusChanged: {
                            if (status === Image.Ready) {
                                if (!photoViewer.isEditing) {
                                    Qt.callLater(function() {
                                        imageContainer.scale = photoViewer.calculateFitScale();
                                    });
                                }
                            }
                        }
                        onSourceSizeChanged: {
                            if (status === Image.Ready) {
                                if (!photoViewer.isEditing) {
                                    Qt.callLater(function() {
                                        imageContainer.scale = photoViewer.calculateFitScale();
                                    });
                                }
                            }
                        }
                    }

                    Image {
                        id: extractedSubjectImage
                        anchors.fill: image
                        fillMode: Image.PreserveAspectFit
                        visible: (source !== "" && !photoViewer.isGif) || photoViewer.subjectHovered
                        z: 1

                        SequentialAnimation on scale {
                            id: subjectAnimation
                            running: false
                            PropertyAnimation { to: 1.05; duration: 200; easing.type: Easing.OutQuad }
                            PropertyAnimation { to: 1.0; duration: 200; easing.type: Easing.InOutQuad }
                        }

                        SequentialAnimation on opacity {
                            id: subjectOpacityAnim
                            running: false
                            PropertyAnimation { from: 0; to: 1; duration: 300 }
                        }
                    }

                    LiveTextOverlay {
                        id: liveTextOverlay
                        anchors.fill: parent
                        ocrModel: photoViewModel.ocrModel
                        imageItem: photoViewer.isGif ? animatedImage : image
                        visible: !photoViewer.isEditing
                        z: 10
                    }
                }

                PinchHandler {
                    target: null
                    property real initialScale: 1.0
                    onActiveChanged: {
                        if (active) {
                            initialScale = imageContainer.scale
                        }
                    }
                    onActiveScaleChanged: {
                        if (active) {
                            const pt = imageContainer.mapToItem(flickable, centroid.position.x, centroid.position.y)
                            photoViewer.zoomToPoint(initialScale * activeScale, pt.x, pt.y, false)
                        }
                    }
                }

                WheelHandler {
                    id: wheelHandler
                    target: null
                    acceptedModifiers: Qt.ControlModifier
                    property real zoomFactor: 1.15
                    onWheel: (event) => {
                        const s1 = imageContainer.scale;
                        const factor = event.angleDelta.y > 0 ? zoomFactor : 1.0 / zoomFactor;
                        const s2 = s1 * factor;
                        const pt = imageContainer.mapToItem(flickable, event.x, event.y)
                        photoViewer.zoomToPoint(s2, pt.x, pt.y, false)
                    }
                }

                TapHandler {
                    onDoubleTapped: {
                        if (imageContainer.scale > 1.0) {
                            photoViewer.zoomToCenter(1.0);
                        } else {
                            photoViewer.zoomToCenter(2.0);
                        }
                    }
                }

                TapHandler {
                    enabled: !photoViewer.isEditing
                    onLongPressed: {
                        if (!photoViewer.isGif) {
                            photoViewModel.extractSubject(photoViewer.sourceUrl)
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    enabled: !photoViewer.isEditing
                    onTapped: (eventPoint) => {
                        photoViewer.selectedOcrText = liveTextOverlay.selectedText
                        contextMenu.popup(eventPoint.scenePosition)
                    }
                }
            }
        }

        GifTimeline {
            id: gifTimeline
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 20
            visible: photoViewer.isGif
            targetImage: animatedImage
        }

        ViewerToolbar {
            id: topBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            isGif: photoViewer.isGif
            isEditorVisible: editor.visible
            currentScale: imageContainer.scale
            isOcrLoading: photoViewModel.isOcrLoading
            onBackClicked: photoViewer.backClicked()
            onRotateClicked: photoViewer.viewRotation = photoViewer.viewRotation + 90
            onEditClicked: {
                // Reset zoom to fit before entering editor
                photoViewer.zoomToCenter(photoViewer.calculateFitScale(), false)
                editor.visible = true
            }
            onZoomRequested: (newScale) => photoViewer.zoomToCenter(newScale)
        }

        NavigationButtons {
            anchors.fill: parent
            hasPrevious: photoViewer.hasPrevious
            hasNext: photoViewer.hasNext
            onPreviousClicked: {
                photoViewer.navDirection = -1
                photoViewer.zoomToCenter(1.0, false)
                photoViewer.sourceUrl = photoViewModel.gallery.galleryImages[photoViewer.currentIndex - 1]
            }
            onNextClicked: {
                photoViewer.navDirection = 1
                photoViewer.zoomToCenter(1.0, false)
                photoViewer.sourceUrl = photoViewModel.gallery.galleryImages[photoViewer.currentIndex + 1]
            }
        }

        PhotoEditor {
            id: editor
            viewer: photoViewer
            z: 100
            anchors.fill: parent
            visible: false
            viewModel: photoViewModel
            sourceUrl: photoViewer.sourceUrl
            imageItem: image
            onImageUpdated: (newUrl) => { photoViewer.sourceUrl = newUrl; }
        }
    }
}
