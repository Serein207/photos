import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: photoEditor
    anchors.fill: parent

    property int rotationAngle: 0
    property string activeTool: "none"
    property var viewModel: null
    property string sourceUrl: ""
    property var imageItem: null // reference to the image element
    property int brushThickness: 5
    property string originalUrl: ""
    property var viewer: null
    property bool cropPending: false
    property real cropAspectRatio: -1

    onVisibleChanged: {
        if (visible) {
            originalUrl = sourceUrl;
            activeTool = "none";
            cropPending = false;
        } else {
            cropAspectRatio = -1;
        }
    }
    
    function executeCrop() {
        if (activeTool === "crop" && cropOverlay.hasBox && imageItem) {
            const imgWidth = imageItem.sourceSize.width || 1;
            const imgHeight = imageItem.sourceSize.height || 1;
            const pWidth = imageItem.paintedWidth || 1;
            const pHeight = imageItem.paintedHeight || 1;
            const xOff = (imageItem.width - pWidth) / 2;
            const yOff = (imageItem.height - pHeight) / 2;

            const pt1 = cropOverlay.mapToItem(imageItem, cropOverlay.boxX, cropOverlay.boxY);
            const pt2 = cropOverlay.mapToItem(imageItem, cropOverlay.boxX + cropOverlay.boxW, cropOverlay.boxY + cropOverlay.boxH);

            const realX = ((pt1.x - xOff) / pWidth) * imgWidth;
            const realY = ((pt1.y - yOff) / pHeight) * imgHeight;
            const realW = ((pt2.x - pt1.x) / pWidth) * imgWidth;
            const realH = ((pt2.y - pt1.y) / pHeight) * imgHeight;

            if (realW > 5 && realH > 5 && viewModel && sourceUrl !== "") {
                const newUrl = viewModel.editor.applyCrop(sourceUrl, realX, realY, realW, realH);
                photoEditor.imageUpdated(newUrl);
            }
        }
        activeTool = "none";
        cancelCrop();
        cropAspectRatio = -1;
    }

    function cancelCrop() {
        cropOverlay.clearBox();
        cropPending = false;
        cropAspectRatio = -1;
    }

    signal imageUpdated(string newUrl)
    
    Item {
        id: toolbar
        width: parent.width
        height: 56
        anchors.top: parent.top
        z: 10

        Rectangle {
            anchors.fill: parent
            color: "#1C1C1E"
            border.color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 0
            // bottom border
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Qt.rgba(1, 1, 1, 0.08)
            }
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

        // Left: zoom controls
        Row {
            id: zoomRow
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 14
            spacing: 0
            visible: photoEditor.viewer !== null

            ZoomPill {
                currentScale: photoEditor.viewer ? photoEditor.viewer.currentScale : 1.0
                onZoomRequested: (s) => { if (photoEditor.viewer) photoEditor.viewer.zoomToCenter(s) }
            }
        }

        // Center: tool buttons
        Row {
            anchors.centerIn: parent
            spacing: 8

            GhostIconButton {
                iconPath: "M15.55 5.55L11 1v3C7.13 4 4 7.13 4 11s3.13 7 7 7c1.53 0 2.96-.49 4.13-1.33l-1.45-1.45C12.91 15.7 12 16 11 16c-2.76 0-5-2.24-5-5s2.24-5 5-5v3l4.55-4.55z"
                onClicked: {
                    if (photoEditor.viewModel && photoEditor.sourceUrl !== "") {
                        const newUrl = photoEditor.viewModel.editor.applyRotation(photoEditor.sourceUrl, 90)
                        photoEditor.imageUpdated(newUrl)
                    }
                }
            }

            GhostIconButton {
                active: photoEditor.activeTool === "crop"
                iconPath: "M17 15h2V7c0-1.1-.9-2-2-2H9v2h8v8zM7 17V1H5v4H1v2h4v10c0 1.1.9 2 2 2h10v4h2v-4h4v-2H7z"
                onClicked: {
                    if (photoEditor.activeTool === "crop") {
                        photoEditor.activeTool = "none"
                        photoEditor.cancelCrop()
                    } else {
                        photoEditor.activeTool = "crop"
                    }
                }
            }

            GhostIconButton {
                opacity: photoEditor.activeTool === "crop" ? 0.35 : 1.0
                active: photoEditor.activeTool === "brush"
                iconPath: "M7 14c-1.66 0-3 1.34-3 3 0 1.31-1.16 2-2 2 .92 1.22 2.49 2 4 2 2.21 0 4-1.79 4-4 0-1.66-1.34-3-3-3zm13.71-9.37l-1.34-1.34c-.39-.39-1.02-.39-1.41 0L9 12.25 11.75 15l8.96-8.96c.39-.39.39-1.02 0-1.41z"
                onClicked: {
                    if (photoEditor.activeTool !== "crop")
                        photoEditor.activeTool = photoEditor.activeTool === "brush" ? "none" : "brush"
                }
            }

            Row {
                visible: photoEditor.activeTool === "brush"
                spacing: 6
                anchors.verticalCenter: parent.verticalCenter

                GhostIconButton {
                    active: photoEditor.brushThickness === 2
                    iconPath: "M 12 10 A 2 2 0 1 1 12 14 A 2 2 0 1 1 12 10"
                    onClicked: photoEditor.brushThickness = 2
                }
                GhostIconButton {
                    active: photoEditor.brushThickness === 5
                    iconPath: "M 12 8 A 4 4 0 1 1 12 16 A 4 4 0 1 1 12 8"
                    onClicked: photoEditor.brushThickness = 5
                }
                GhostIconButton {
                    active: photoEditor.brushThickness === 10
                    iconPath: "M 12 6 A 6 6 0 1 1 12 18 A 6 6 0 1 1 12 6"
                    onClicked: photoEditor.brushThickness = 10
                }
            }
        }

        // Right: Save + Discard
        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 14
            spacing: 8

            GhostIconButton {
                destructive: true
                iconPath: "M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"
                onClicked: {
                    photoEditor.activeTool = "none"
                    photoEditor.imageUpdated(photoEditor.originalUrl)
                    photoEditor.visible = false
                }
            }

            Rectangle {
                width: 34; height: 34
                radius: 8
                color: saveMouse.pressed ? "#0066DD" : "#007AFF"
                Shape {
                    anchors.centerIn: parent
                    width: 24; height: 24
                    ShapePath {
                        fillColor: "#FFFFFF"
                        strokeColor: "transparent"
                        PathSvg { path: "M17 3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V5h10v4z" }
                    }
                }
                MouseArea {
                    id: saveMouse
                    anchors.fill: parent
                    onClicked: {
                        photoEditor.activeTool = "none"
                        photoEditor.visible = false
                    }
                }
            }
        }
    }

    // ── Crop overlay (QML-based, replaces Canvas for crop) ──────────────────
    Item {
        id: cropOverlay
        anchors.fill: parent
        visible: photoEditor.activeTool === "crop"
        z: 4

        // Crop box geometry (normalised so x1<x2, y1<y2)
        property real boxX: 0
        property real boxY: 0
        property real boxW: 0
        property real boxH: 0
        property bool hasBox: boxW > 5 && boxH > 5

        // Reset when crop mode is cancelled externally
        function clearBox() {
            boxX = 0; boxY = 0; boxW = 0; boxH = 0;
        }

        // ── Dark overlay: 4 rectangles around the crop box ──────────────────
        // Top
        Rectangle {
            x: 0; y: 0
            width: parent.width
            height: cropOverlay.hasBox ? cropOverlay.boxY : parent.height
            color: Qt.rgba(0, 0, 0, 0.55)
            visible: cropOverlay.hasBox
        }
        // Bottom
        Rectangle {
            x: 0
            y: cropOverlay.hasBox ? cropOverlay.boxY + cropOverlay.boxH : parent.height
            width: parent.width
            height: cropOverlay.hasBox ? parent.height - (cropOverlay.boxY + cropOverlay.boxH) : 0
            color: Qt.rgba(0, 0, 0, 0.55)
            visible: cropOverlay.hasBox
        }
        // Left
        Rectangle {
            x: 0
            y: cropOverlay.hasBox ? cropOverlay.boxY : 0
            width: cropOverlay.hasBox ? cropOverlay.boxX : 0
            height: cropOverlay.hasBox ? cropOverlay.boxH : 0
            color: Qt.rgba(0, 0, 0, 0.55)
            visible: cropOverlay.hasBox
        }
        // Right
        Rectangle {
            x: cropOverlay.hasBox ? cropOverlay.boxX + cropOverlay.boxW : parent.width
            y: cropOverlay.hasBox ? cropOverlay.boxY : 0
            width: cropOverlay.hasBox ? parent.width - (cropOverlay.boxX + cropOverlay.boxW) : 0
            height: cropOverlay.hasBox ? cropOverlay.boxH : 0
            color: Qt.rgba(0, 0, 0, 0.55)
            visible: cropOverlay.hasBox
        }

        // ── Crop box border ──────────────────────────────────────────────────
        Rectangle {
            x: cropOverlay.boxX
            y: cropOverlay.boxY
            width: cropOverlay.boxW
            height: cropOverlay.boxH
            visible: cropOverlay.hasBox
            color: "transparent"
            border.color: "white"
            border.width: 1

            // Rule-of-thirds grid lines
            Rectangle { x: parent.width/3;   y: 0; width: 1; height: parent.height; color: Qt.rgba(1,1,1,0.3) }
            Rectangle { x: 2*parent.width/3; y: 0; width: 1; height: parent.height; color: Qt.rgba(1,1,1,0.3) }
            Rectangle { x: 0; y: parent.height/3;   width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.3) }
            Rectangle { x: 0; y: 2*parent.height/3; width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.3) }
        }

        // ── Corner handles ───────────────────────────────────────────────────
        readonly property int hSize: 24   // hit area
        readonly property int hVis: 14    // visible L-bracket arm length
        readonly property int hThick: 3   // bracket thickness

        function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)) }

        function applyRatio(x1, y1, x2, y2) {
            if (photoEditor.cropAspectRatio <= 0) return Qt.rect(x1, y1, x2 - x1, y2 - y1);
            const w = Math.abs(x2 - x1);
            const h = w / photoEditor.cropAspectRatio;
            return Qt.rect(x1, y1, x2 - x1, (y2 >= y1) ? h : -h);
        }

        // TL corner
        Rectangle {
            id: tlHandle
            x: cropOverlay.boxX - cropOverlay.hSize / 2
            y: cropOverlay.boxY - cropOverlay.hSize / 2
            width: cropOverlay.hSize; height: cropOverlay.hSize
            color: "transparent"
            visible: cropOverlay.hasBox
            Rectangle { x: 0; y: 0; width: cropOverlay.hVis; height: cropOverlay.hThick; color: "white" }
            Rectangle { x: 0; y: 0; width: cropOverlay.hThick; height: cropOverlay.hVis; color: "white" }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeFDiagCursor
                property real origX2; property real origY2
                onPressed: {
                    origX2 = cropOverlay.boxX + cropOverlay.boxW;
                    origY2 = cropOverlay.boxY + cropOverlay.boxH;
                }
                onPositionChanged: (m) => {
                    if (!pressed) return;
                    const pos = mapToItem(cropOverlay, m.x, m.y);
                    const mx = cropOverlay.clamp(pos.x, 0, origX2 - 10);
                    const my = cropOverlay.clamp(pos.y, 0, origY2 - 10);
                    const r = cropOverlay.applyRatio(mx, my, origX2, origY2);
                    cropOverlay.boxX = origX2 - r.width;
                    cropOverlay.boxY = origY2 - r.height;
                    cropOverlay.boxW = Math.abs(r.width);
                    cropOverlay.boxH = Math.abs(r.height);
                    photoEditor.cropPending = true;
                }
            }
        }

        // TR corner
        Rectangle {
            id: trHandle
            x: cropOverlay.boxX + cropOverlay.boxW - cropOverlay.hSize / 2
            y: cropOverlay.boxY - cropOverlay.hSize / 2
            width: cropOverlay.hSize; height: cropOverlay.hSize
            color: "transparent"
            visible: cropOverlay.hasBox
            Rectangle { x: cropOverlay.hSize - cropOverlay.hVis; y: 0; width: cropOverlay.hVis; height: cropOverlay.hThick; color: "white" }
            Rectangle { x: cropOverlay.hSize - cropOverlay.hThick; y: 0; width: cropOverlay.hThick; height: cropOverlay.hVis; color: "white" }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeBDiagCursor
                property real origX1; property real origY2
                onPressed: {
                    origX1 = cropOverlay.boxX;
                    origY2 = cropOverlay.boxY + cropOverlay.boxH;
                }
                onPositionChanged: (m) => {
                    if (!pressed) return;
                    const pos = mapToItem(cropOverlay, m.x, m.y);
                    const mx = cropOverlay.clamp(pos.x, origX1 + 10, cropOverlay.width);
                    const my = cropOverlay.clamp(pos.y, 0, origY2 - 10);
                    const r = cropOverlay.applyRatio(origX1, my, mx, origY2);
                    cropOverlay.boxX = origX1;
                    cropOverlay.boxY = origY2 - r.height;
                    cropOverlay.boxW = Math.abs(r.width);
                    cropOverlay.boxH = Math.abs(r.height);
                    photoEditor.cropPending = true;
                }
            }
        }

        // BL corner
        Rectangle {
            id: blHandle
            x: cropOverlay.boxX - cropOverlay.hSize / 2
            y: cropOverlay.boxY + cropOverlay.boxH - cropOverlay.hSize / 2
            width: cropOverlay.hSize; height: cropOverlay.hSize
            color: "transparent"
            visible: cropOverlay.hasBox
            Rectangle { x: 0; y: cropOverlay.hSize - cropOverlay.hThick; width: cropOverlay.hVis; height: cropOverlay.hThick; color: "white" }
            Rectangle { x: 0; y: cropOverlay.hSize - cropOverlay.hVis; width: cropOverlay.hThick; height: cropOverlay.hVis; color: "white" }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeBDiagCursor
                property real origX2; property real origY1
                onPressed: {
                    origX2 = cropOverlay.boxX + cropOverlay.boxW;
                    origY1 = cropOverlay.boxY;
                }
                onPositionChanged: (m) => {
                    if (!pressed) return;
                    const pos = mapToItem(cropOverlay, m.x, m.y);
                    const mx = cropOverlay.clamp(pos.x, 0, origX2 - 10);
                    const my = cropOverlay.clamp(pos.y, origY1 + 10, cropOverlay.height);
                    const r = cropOverlay.applyRatio(mx, origY1, origX2, my);
                    cropOverlay.boxX = origX2 - r.width;
                    cropOverlay.boxY = origY1;
                    cropOverlay.boxW = Math.abs(r.width);
                    cropOverlay.boxH = Math.abs(r.height);
                    photoEditor.cropPending = true;
                }
            }
        }

        // BR corner
        Rectangle {
            id: brHandle
            x: cropOverlay.boxX + cropOverlay.boxW - cropOverlay.hSize / 2
            y: cropOverlay.boxY + cropOverlay.boxH - cropOverlay.hSize / 2
            width: cropOverlay.hSize; height: cropOverlay.hSize
            color: "transparent"
            visible: cropOverlay.hasBox
            Rectangle { x: cropOverlay.hSize - cropOverlay.hVis; y: cropOverlay.hSize - cropOverlay.hThick; width: cropOverlay.hVis; height: cropOverlay.hThick; color: "white" }
            Rectangle { x: cropOverlay.hSize - cropOverlay.hThick; y: cropOverlay.hSize - cropOverlay.hVis; width: cropOverlay.hThick; height: cropOverlay.hVis; color: "white" }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeFDiagCursor
                property real origX1; property real origY1
                onPressed: {
                    origX1 = cropOverlay.boxX;
                    origY1 = cropOverlay.boxY;
                }
                onPositionChanged: (m) => {
                    if (!pressed) return;
                    const pos = mapToItem(cropOverlay, m.x, m.y);
                    const mx = cropOverlay.clamp(pos.x, origX1 + 10, cropOverlay.width);
                    const my = cropOverlay.clamp(pos.y, origY1 + 10, cropOverlay.height);
                    const r = cropOverlay.applyRatio(origX1, origY1, mx, my);
                    cropOverlay.boxX = origX1;
                    cropOverlay.boxY = origY1;
                    cropOverlay.boxW = Math.abs(r.width);
                    cropOverlay.boxH = Math.abs(r.height);
                    photoEditor.cropPending = true;
                }
            }
        }

        // ── Initial draw MouseArea (background, below handles) ───────────────
        MouseArea {
            id: cropDrawArea
            anchors.fill: parent
            z: -1   // below corner handles
            preventStealing: true
            property real startX; property real startY

            onPressed: (mouse) => {
                // If clicking outside the existing box, start a new one
                startX = mouse.x;
                startY = mouse.y;
                cropOverlay.boxX = mouse.x;
                cropOverlay.boxY = mouse.y;
                cropOverlay.boxW = 0;
                cropOverlay.boxH = 0;
                photoEditor.cropPending = false;
            }
            onPositionChanged: (mouse) => {
                if (!pressed) return;
                let x1 = Math.min(startX, mouse.x);
                let y1 = Math.min(startY, mouse.y);
                let x2 = Math.max(startX, mouse.x);
                let y2 = Math.max(startY, mouse.y);
                if (photoEditor.cropAspectRatio > 0) {
                    const w = x2 - x1;
                    y2 = y1 + w / photoEditor.cropAspectRatio;
                }
                cropOverlay.boxX = x1;
                cropOverlay.boxY = y1;
                cropOverlay.boxW = x2 - x1;
                cropOverlay.boxH = y2 - y1;
            }
            onReleased: (mouse) => {
                if (cropOverlay.boxW > 5 && cropOverlay.boxH > 5) {
                    photoEditor.cropPending = true;
                } else {
                    cropOverlay.clearBox();
                    photoEditor.cropPending = false;
                }
            }
        }
    }

    // ── Brush Canvas ─────────────────────────────────────────────────────────
    Canvas {
        id: brushCanvas
        anchors.fill: parent
        visible: photoEditor.activeTool === "brush"
        z: 4

        property var strokePoints: []
        property var mappedPoints: []

        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            if (strokePoints.length > 0) {
                ctx.strokeStyle = "red";
                ctx.lineWidth = photoEditor.brushThickness;
                ctx.lineCap = "round";
                ctx.lineJoin = "round";
                ctx.beginPath();
                ctx.moveTo(strokePoints[0].x, strokePoints[0].y);
                for (let i = 1; i < strokePoints.length; i++)
                    ctx.lineTo(strokePoints[i].x, strokePoints[i].y);
                ctx.stroke();
            }
        }

        MouseArea {
            anchors.fill: parent
            preventStealing: true
            onPressed: (mouse) => {
                brushCanvas.strokePoints = [{x: mouse.x, y: mouse.y}];
                brushCanvas.mappedPoints = [];
                addMappedPoint(mouse.x, mouse.y);
                brushCanvas.requestPaint();
            }
            onPositionChanged: (mouse) => {
                if (!pressed) return;
                brushCanvas.strokePoints.push({x: mouse.x, y: mouse.y});
                addMappedPoint(mouse.x, mouse.y);
                brushCanvas.requestPaint();
            }
            onReleased: (mouse) => {
                if (brushCanvas.mappedPoints.length > 0 && photoEditor.viewModel && photoEditor.sourceUrl !== "") {
                    const newUrl = photoEditor.viewModel.editor.applyBrush(
                        photoEditor.sourceUrl, brushCanvas.mappedPoints, "#ff0000", photoEditor.brushThickness);
                    photoEditor.imageUpdated(newUrl);
                }
                brushCanvas.strokePoints = [];
                brushCanvas.mappedPoints = [];
                const ctx = brushCanvas.getContext("2d");
                ctx.clearRect(0, 0, brushCanvas.width, brushCanvas.height);
                brushCanvas.requestPaint();
            }

            function addMappedPoint(mx, my) {
                if (!photoEditor.imageItem) return;
                const imgW = photoEditor.imageItem.sourceSize.width || 1;
                const imgH = photoEditor.imageItem.sourceSize.height || 1;
                const pW = photoEditor.imageItem.paintedWidth || 1;
                const pH = photoEditor.imageItem.paintedHeight || 1;
                const xOff = (photoEditor.imageItem.width - pW) / 2;
                const yOff = (photoEditor.imageItem.height - pH) / 2;
                const pt = brushCanvas.mapToItem(photoEditor.imageItem, mx, my);
                brushCanvas.mappedPoints.push(Qt.point(
                    ((pt.x - xOff) / pW) * imgW,
                    ((pt.y - yOff) / pH) * imgH
                ));
            }
        }
    }

    CropActionBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 56
        z: 5
        cropPending: photoEditor.cropPending
        activeTool: photoEditor.activeTool
        cropAspectRatio: photoEditor.cropAspectRatio
        onConfirmCrop: photoEditor.executeCrop()
        onCancelCrop: photoEditor.cancelCrop()
        onAspectRatioRequested: (ratio) => {
            cropOverlay.clearBox();
            photoEditor.cropPending = false;
            photoEditor.cropAspectRatio = ratio;
        }
    }
}
