import QtQuick

Item {
    id: liveTextOverlay
    anchors.fill: parent

    required property var ocrModel
    required property var imageItem

    property var charBoxes: []
    property var charLineMap: []
    property var lineStartMap: ({})
    property var lineEndMap: ({})
    property int selStart: -1
    property int selEnd: -1
    property string selectedText: ""

    function refreshCharBoxes() {
        const boxes = ocrModel ? ocrModel.allCharBoxes() : []
        charBoxes = []
        charBoxes = boxes
    }

    Component.onCompleted: {
        refreshCharBoxes()
        if (ocrModel) ocrModel.modelReset.connect(refreshCharBoxes)
    }

    onOcrModelChanged: {
        refreshCharBoxes()
        if (ocrModel) ocrModel.modelReset.connect(refreshCharBoxes)
    }

    property var lineBBoxes: []   // [{minX, minY, maxX, maxY, angle}] in image coords, one per line

    onCharBoxesChanged: {
        const map = new Array(charBoxes.length)
        const starts = {}
        const ends = {}
        for (let i = 0; i < charBoxes.length; ++i) {
            const li = charBoxes[i].lineIndex !== undefined ? charBoxes[i].lineIndex : 0
            map[i] = li
            if (starts[li] === undefined) starts[li] = i
            ends[li] = i
        }
        charLineMap = map
        lineStartMap = starts
        lineEndMap = ends

        const bboxes = []
        for (const li in starts) {
            let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity
            let sumSin = 0, sumCos = 0, count = 0
            for (let i = starts[li]; i <= ends[li]; ++i) {
                const b = charBoxes[i]
                const halfW = (b.w || b.bh || 8) / 2
                const halfH = (b.h || b.bh || 8) / 2
                if (b.cx - halfW < minX) minX = b.cx - halfW
                if (b.cx + halfW > maxX) maxX = b.cx + halfW
                if (b.cy - halfH < minY) minY = b.cy - halfH
                if (b.cy + halfH > maxY) maxY = b.cy + halfH
                const rad = (b.angle !== undefined ? b.angle : 0) * Math.PI / 180
                sumSin += Math.sin(rad)
                sumCos += Math.cos(rad)
                count++
            }
            const charCount = ends[li] - starts[li] + 1
            if (charCount > 1) {
                const halfSlot = (maxX - minX) / (charCount - 1) / 2
                minX -= halfSlot
                maxX += halfSlot
            }
            const avgAngle = count ? Math.atan2(sumSin, sumCos) * 180 / Math.PI : 0
            bboxes.push({minX, minY, maxX, maxY, angle: avgAngle})
        }
        lineBBoxes = bboxes

        selStart = -1
        selEnd = -1
        selectedText = ""
        highlightCanvas.requestPaint()
    }

    function overlayToImage(ox, oy) {
        const img = imageItem
        if (!img) return Qt.point(ox, oy)
        const imgW = img.sourceSize.width || 1
        const imgH = img.sourceSize.height || 1
        const pW = img.paintedWidth || 1
        const pH = img.paintedHeight || 1
        const xOff = (img.width - pW) / 2
        const yOff = (img.height - pH) / 2
        return Qt.point(((ox - xOff) / pW) * imgW, ((oy - yOff) / pH) * imgH)
    }

    function findNearestChar(imgX, imgY) {
        if (charBoxes.length === 0) return -1

        // Find the nearest char by 2D distance, with Y weighted more than X
        // so horizontal drag stays on the same line, but moving vertically
        // switches lines naturally.
        let bestIdx = 0
        let bestDist = Infinity
        for (let i = 0; i < charBoxes.length; ++i) {
            const dx = charBoxes[i].cx - imgX
            const dy = (charBoxes[i].cy - imgY) * 2.0   // weight Y so lines don't bleed into each other
            const d = dx * dx + dy * dy
            if (d < bestDist) { bestDist = d; bestIdx = i }
        }
        return bestIdx
    }

    function buildSelectedText() {
        if (selStart < 0 || selEnd < 0) return ""
        const lo = Math.min(selStart, selEnd)
        const hi = Math.max(selStart, selEnd)
        let text = ""
        for (let i = lo; i <= hi && i < charBoxes.length; ++i)
            text += charBoxes[i].ch
        return text
    }

    Canvas {
        id: highlightCanvas
        anchors.fill: parent
        z: 10

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            if (liveTextOverlay.selStart < 0 || liveTextOverlay.selEnd < 0) return

            const lo = Math.min(liveTextOverlay.selStart, liveTextOverlay.selEnd)
            const hi = Math.max(liveTextOverlay.selStart, liveTextOverlay.selEnd)
            const boxes = liveTextOverlay.charBoxes
            if (boxes.length === 0) return

            const img = liveTextOverlay.imageItem
            if (!img) return
            const imgW = img.sourceSize.width || 1
            const imgH = img.sourceSize.height || 1
            const pW = img.paintedWidth || 1
            const pH = img.paintedHeight || 1
            const xOff = (img.width - pW) / 2
            const yOff = (img.height - pH) / 2
            const scaleX = pW / imgW
            const scaleY = pH / imgH

            const lineMap = liveTextOverlay.charLineMap
            const starts  = liveTextOverlay.lineStartMap
            const ends    = liveTextOverlay.lineEndMap

            const firstLine = lineMap[lo]
            const lastLine  = lineMap[hi]

            const ranges = []
            if (firstLine === lastLine) {
                ranges.push([lo, hi])
            } else {
                ranges.push([lo, ends[firstLine]])
                for (let li = firstLine + 1; li < lastLine; ++li) {
                    if (starts[li] !== undefined)
                        ranges.push([starts[li], ends[li]])
                }
                ranges.push([starts[lastLine], hi])
            }

            ctx.fillStyle = "#500080FF"
            for (let r = 0; r < ranges.length; ++r) {
                const rlo = ranges[r][0], rhi = ranges[r][1]
                const lineIdx = lineMap[rlo]
                const lineS = starts[lineIdx]
                const lineE = ends[lineIdx]

                // get per-line rotation angle
                const lineAngle = (liveTextOverlay.lineBBoxes[lineIdx]
                                   && liveTextOverlay.lineBBoxes[lineIdx].angle !== undefined)
                                   ? liveTextOverlay.lineBBoxes[lineIdx].angle : 0

                let minY = Infinity, maxY = -Infinity
                for (let j = lineS; j <= lineE && j < boxes.length; ++j) {
                    const cb2 = boxes[j]
                    const cy2 = yOff + cb2.cy * scaleY
                    const hh2 = ((cb2.h || cb2.bh || 8) * scaleY) / 2 * 1.4
                    if (cy2 - hh2 < minY) minY = cy2 - hh2
                    if (cy2 + hh2 > maxY) maxY = cy2 + hh2
                }

                const cxLo = xOff + boxes[rlo].cx * scaleX
                let minX
                if (rlo > lineS) {
                    const cxPrev = xOff + boxes[rlo - 1].cx * scaleX
                    minX = (cxPrev + cxLo) / 2
                } else if (rlo < lineE) {
                    const cxNext0 = xOff + boxes[rlo + 1].cx * scaleX
                    minX = cxLo - (cxNext0 - cxLo) / 2
                } else {
                    minX = cxLo - ((boxes[rlo].w || boxes[rlo].bh || 8) * scaleX) / 2
                }

                const cxHi = xOff + boxes[rhi].cx * scaleX
                let maxX
                if (rhi < lineE) {
                    const cxNext = xOff + boxes[rhi + 1].cx * scaleX
                    maxX = (cxHi + cxNext) / 2
                } else if (rhi > lineS) {
                    const cxPrev2 = xOff + boxes[rhi - 1].cx * scaleX
                    maxX = cxHi + (cxHi - cxPrev2) / 2
                } else {
                    maxX = cxHi + ((boxes[rhi].w || boxes[rhi].bh || 8) * scaleX) / 2
                }

                if (minX < maxX && minY < maxY) {
                    const cx = (minX + maxX) / 2
                    const cy = (minY + maxY) / 2
                    ctx.save()
                    ctx.translate(cx, cy)
                    if (Math.abs(lineAngle) > 0.5)
                        ctx.rotate(lineAngle * Math.PI / 180)
                    ctx.fillRect(-(maxX - minX) / 2, -(maxY - minY) / 2,
                                  maxX - minX, maxY - minY)
                    ctx.restore()
                }
            }
        }
    }

    MouseArea {
        id: textSelectArea
        anchors.fill: parent
        z: 11
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        propagateComposedEvents: true
        preventStealing: pressed && liveTextOverlay.charBoxes.length > 0

        cursorShape: textSelectArea.overText ? Qt.IBeamCursor : Qt.ArrowCursor

        property int anchorIdx: -1
        property bool didDrag: false
        property bool overText: false

        function updateOverText(mx, my) {
            if (liveTextOverlay.lineBBoxes.length === 0) { overText = false; return }
            const imgPt = liveTextOverlay.overlayToImage(mx, my)
            const bboxes = liveTextOverlay.lineBBoxes
            for (let i = 0; i < bboxes.length; ++i) {
                const bb = bboxes[i]
                if (imgPt.x >= bb.minX && imgPt.x <= bb.maxX &&
                    imgPt.y >= bb.minY && imgPt.y <= bb.maxY) {
                    overText = true; return
                }
            }
            overText = false
        }

        onPressed: (mouse) => {
            if (liveTextOverlay.charBoxes.length === 0) {
                mouse.accepted = false
                return
            }
            const imgPt = liveTextOverlay.overlayToImage(mouse.x, mouse.y)
            anchorIdx = liveTextOverlay.findNearestChar(imgPt.x, imgPt.y)
            didDrag = false
        }

        onPositionChanged: (mouse) => {
            updateOverText(mouse.x, mouse.y)
            if (!pressed || anchorIdx < 0) return
            const imgPt = liveTextOverlay.overlayToImage(mouse.x, mouse.y)
            const cur = liveTextOverlay.findNearestChar(imgPt.x, imgPt.y)
            if (cur < 0) return
            didDrag = true
            liveTextOverlay.selStart = anchorIdx
            liveTextOverlay.selEnd = cur
            highlightCanvas.requestPaint()
        }

        onReleased: (mouse) => {
            if (anchorIdx < 0) return
            const imgPt = liveTextOverlay.overlayToImage(mouse.x, mouse.y)
            let endIdx = liveTextOverlay.findNearestChar(imgPt.x, imgPt.y)
            if (endIdx < 0) endIdx = anchorIdx

            if (didDrag) {
                liveTextOverlay.selStart = anchorIdx
                liveTextOverlay.selEnd = endIdx
                liveTextOverlay.selectedText = liveTextOverlay.buildSelectedText()
            } else {
                liveTextOverlay.selStart = -1
                liveTextOverlay.selEnd = -1
                liveTextOverlay.selectedText = ""
            }
            anchorIdx = -1
            highlightCanvas.requestPaint()
        }

        onExited: overText = false

        onDoubleClicked: (mouse) => {
            if (liveTextOverlay.charBoxes.length > 0) {
                liveTextOverlay.selStart = 0
                liveTextOverlay.selEnd = liveTextOverlay.charBoxes.length - 1
                liveTextOverlay.selectedText = liveTextOverlay.buildSelectedText()
                highlightCanvas.requestPaint()
            }
        }
    }
}
