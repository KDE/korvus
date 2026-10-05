// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

// A MapView tuned for touch, based on QtLocation's MapView.qml
//  - follows a coordinate until the user moves the map, a button resumes following
//  - pinch rotation only kicks in after a deliberate twist, no accidental tilt
//  - no flick after a pinch, so lifting the fingers doesn't make the map drift
//  - double tap zooms in, overlay buttons for zoom, north and recenter
Item {
    id: root

    property alias map: map
    // The coordinate to keep centered while following is true
    property var followCoordinate: QtPositioning.coordinate()
    property bool following: true
    readonly property int touchTarget: 48

    // Degrees the fingers have to rotate before a pinch also rotates the map
    readonly property real rotationThreshold: 20
    // Degrees around north where the bearing snaps to 0
    readonly property real bearingSnap: 5

    function centerOn(coordinate) {
        if (!coordinate || !coordinate.isValid) {
            return;
        }
        centerAnimation.stop();
        centerAnimation.from = map.center;
        centerAnimation.to = coordinate;
        centerAnimation.start();
    }

    // Zooms by delta while keeping the coordinate under point in place
    function zoomAt(delta, point) {
        zoomAnimation.stop();
        zoomAnimation.anchorPoint = point;
        zoomAnimation.anchorCoordinate = map.toCoordinate(point, false);
        zoomAnimation.to = Math.max(map.minimumZoomLevel, Math.min(map.maximumZoomLevel, map.zoomLevel + delta));
        zoomAnimation.start();
    }

    function resetBearing() {
        bearingAnimation.stop();
        // rotate the short way round
        bearingAnimation.to = map.bearing > 180 ? 360 : 0;
        bearingAnimation.start();
    }

    // Called whenever the user starts manipulating the map
    function userInteraction() {
        root.following = false;
        centerAnimation.stop();
        zoomAnimation.stop();
        bearingAnimation.stop();
        flickAnimation.stop();
    }

    // Whether the map was ever centered on a valid followCoordinate, until then jump instead of animating from 0,0
    property bool _centered: false

    function _follow() {
        if (!following || !followCoordinate.isValid) {
            return;
        }
        if (_centered) {
            centerOn(followCoordinate);
        } else {
            map.center = followCoordinate;
            _centered = true;
        }
    }

    onFollowCoordinateChanged: _follow()
    onFollowingChanged: _follow()

    Component.onCompleted: _follow()

    Map {
        id: map
        anchors.fill: parent

        onZoomLevelChanged: {
            // while following the center belongs to the follow animation
            if (zoomAnimation.running && !root.following) {
                map.alignCoordinateToPoint(zoomAnimation.anchorCoordinate, zoomAnimation.anchorPoint);
            }
        }

        PinchHandler {
            id: pinch
            target: null
            // take the first finger over from an active DragHandler when the second one lands,
            // TakeOverForbidden alone would leave the pinch waiting for two fingers nobody grabbed yet
            grabPermissions: PointerHandler.CanTakeOverFromHandlersOfDifferentType | PointerHandler.CanTakeOverFromItems
            // react as soon as the fingers move, the default threshold swallows the start of the gesture
            dragThreshold: 2

            property geoCoordinate startCentroid
            property real startZoom: 0
            property real startBearing: 0
            // activeRotation at the moment the rotation threshold was crossed
            property real rotationOffset: 0
            property bool rotating: false

            // Zoom and rotation are derived from the totals of the gesture rather than summed up deltas,
            // so clamping at the zoom limits or ignoring small twists doesn't make the map lag behind the fingers
            function update() {
                if (!active) {
                    return;
                }
                map.zoomLevel = Math.max(map.minimumZoomLevel, Math.min(map.maximumZoomLevel, startZoom + Math.log2(activeScale)));
                if (!rotating && Math.abs(activeRotation) >= root.rotationThreshold) {
                    // start from the current finger angle so the map doesn't jump by the threshold
                    rotating = true;
                    rotationOffset = activeRotation;
                }
                if (rotating) {
                    const bearing = ((startBearing - (activeRotation - rotationOffset)) % 360 + 360) % 360;
                    map.bearing = (bearing < root.bearingSnap || bearing > 360 - root.bearingSnap) ? 0 : bearing;
                }
                map.alignCoordinateToPoint(startCentroid, centroid.position);
            }

            onActiveChanged: {
                if (active) {
                    root.userInteraction();
                    startCentroid = map.toCoordinate(centroid.position, false);
                    startZoom = map.zoomLevel;
                    startBearing = map.bearing;
                    rotating = false;
                }
            }
            onScaleChanged: update()
            onRotationChanged: update()
            onTranslationChanged: update()
        }

        WheelHandler {
            id: wheel
            // workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432, see QtLocation's MapView.qml
            acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland" ? PointerDevice.Mouse | PointerDevice.TouchPad : PointerDevice.Mouse
            onWheel: event => {
                root.userInteraction();
                // two finger scrolling on a trackpad pans like in other map apps, pinching zooms through the PinchHandler
                if (event.modifiers === Qt.NoModifier && (event.pixelDelta.x !== 0 || event.pixelDelta.y !== 0) && event.device.type === PointerDevice.TouchPad) {
                    map.pan(-event.pixelDelta.x, -event.pixelDelta.y);
                    return;
                }
                const loc = map.toCoordinate(wheel.point.position);
                if (event.modifiers === Qt.ShiftModifier) {
                    map.bearing += event.angleDelta.y / 15;
                } else {
                    map.zoomLevel += event.angleDelta.y / 120;
                }
                map.alignCoordinateToPoint(loc, wheel.point.position);
            }
        }

        DragHandler {
            id: drag
            target: null
            onTranslationChanged: delta => map.pan(-delta.x, -delta.y)
            onActiveChanged: {
                if (active) {
                    root.userInteraction();
                } else if (!pinch.active) {
                    flickAnimation.restart(centroid.velocity);
                }
            }
        }

        TapHandler {
            onDoubleTapped: eventPoint => {
                root.userInteraction();
                root.zoomAt(1, eventPoint.position);
            }
        }

        property vector3d animDest
        onAnimDestChanged: {
            if (flickAnimation.running) {
                const delta = Qt.vector2d(animDest.x - flickAnimation.animDestLast.x, animDest.y - flickAnimation.animDestLast.y);
                map.pan(-delta.x, -delta.y);
                flickAnimation.animDestLast = animDest;
            }
        }

        Vector3dAnimation on animDest {
            id: flickAnimation
            property vector3d animDestLast
            from: Qt.vector3d(0, 0, 0)
            duration: 500
            easing.type: Easing.OutQuad
            running: false

            function restart(vel) {
                stop();
                map.animDest = Qt.vector3d(0, 0, 0);
                animDestLast = Qt.vector3d(0, 0, 0);
                to = Qt.vector3d(vel.x / duration * 100, vel.y / duration * 100, 0);
                start();
            }
        }

        CoordinateAnimation {
            id: centerAnimation
            target: map
            property: "center"
            duration: 400
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            id: zoomAnimation
            property point anchorPoint
            property geoCoordinate anchorCoordinate
            target: map
            property: "zoomLevel"
            duration: 250
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            id: bearingAnimation
            target: map
            property: "bearing"
            duration: 300
            easing.type: Easing.OutCubic
        }
    }

    Column {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        spacing: 8

        RoundButton {
            width: root.touchTarget
            height: root.touchTarget
            visible: map.bearing !== 0
            text: "N"
            font.bold: true
            // point the label to where north is
            contentItem.rotation: -map.bearing
            Accessible.name: qsTr("Reset rotation")
            onClicked: root.resetBearing()
        }

        RoundButton {
            width: root.touchTarget
            height: root.touchTarget
            text: "+"
            font.pixelSize: Qt.application.font.pixelSize * 1.6
            enabled: map.zoomLevel < map.maximumZoomLevel
            Accessible.name: qsTr("Zoom in")
            onClicked: root.zoomAt(1, Qt.point(map.width / 2, map.height / 2))
        }

        RoundButton {
            width: root.touchTarget
            height: root.touchTarget
            text: "−"
            font.pixelSize: Qt.application.font.pixelSize * 1.6
            enabled: map.zoomLevel > map.minimumZoomLevel
            Accessible.name: qsTr("Zoom out")
            onClicked: root.zoomAt(-1, Qt.point(map.width / 2, map.height / 2))
        }

        RoundButton {
            width: root.touchTarget
            height: root.touchTarget
            text: "◎"
            font.pixelSize: Qt.application.font.pixelSize * 1.4
            highlighted: root.following
            enabled: root.followCoordinate.isValid
            Accessible.name: qsTr("Follow my position")
            onClicked: root.following = true
        }
    }
}
