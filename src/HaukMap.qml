// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtLocation
import QtPositioning

import org.kde.hauk 1.0
import org.kde.korvus

Page {
    id: page
    title: session.isSharing ? session.shareType === QHaukSession.Solo ? qsTr("ShareID: %1").arg(session.shareId) : qsTr("ShareID: %1 PIN: %2").arg(session.shareId).arg(session.pin) : qsTr("The session has ended")

    required property QHaukSession session
    readonly property int touchTarget: 48
    readonly property int pageMargin: 16
    readonly property int maxContentWidth: 600

    // Trail of past positions of one member
    component MemberTrail: MapPolyline {
        required property QHaukPositionHistory history

        z: 1
        visible: history.positions.length > 1
        path: history.positions.map(p => p)
        line.width: 3
        line.color: history.color
        opacity: 0.7
    }

    // Marker placed at the latest known position of one member
    component MemberMarker: MapQuickItem {
        id: memberItem
        required property QHaukPositionHistory history
        required property Map map

        readonly property var lastPosition: history.lastKnownPosition
        readonly property bool isMoving: history.speed > 0
        // Accuracy (meters) converted to screen pixels, re-evaluated whenever the zoom changes
        readonly property real accuracyRadius: {
            map.zoomLevel;
            if (history.accuracy < 0) {
                return 0;
            }
            const center = map.fromCoordinate(lastPosition, false);
            const edge = map.fromCoordinate(lastPosition.atDistanceAndAzimuth(history.accuracy, 90), false);
            return Math.hypot(edge.x - center.x, edge.y - center.y);
        }

        z: 2
        visible: history.positions.length > 0
        coordinate: lastPosition
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2

        sourceItem: Item {
            width: Math.max(marker.width, accuracyCircle.width)
            height: width

            Rectangle {
                id: accuracyCircle
                anchors.centerIn: parent
                width: memberItem.accuracyRadius * 2
                height: width
                radius: width / 2
                visible: width > marker.width
                color: Qt.rgba(memberItem.history.color.r, memberItem.history.color.g, memberItem.history.color.b, 0.2)
                border.color: Qt.rgba(memberItem.history.color.r, memberItem.history.color.g, memberItem.history.color.b, 0.5)
                border.width: 1
            }

            Item {
                id: marker
                anchors.centerIn: parent
                width: memberItem.isMoving ? 32 : 16
                height: width

                Rectangle {
                    anchors.fill: parent
                    visible: !memberItem.isMoving
                    radius: width / 2
                    color: memberItem.history.color
                    border.color: "white"
                    border.width: 2
                }

                // Arrow pointing up (north), rotated to the heading relative to the map bearing
                Shape {
                    anchors.fill: parent
                    visible: memberItem.isMoving
                    rotation: memberItem.history.direction - memberItem.map.bearing
                    preferredRendererType: Shape.CurveRenderer

                    ShapePath {
                        fillColor: memberItem.history.color
                        strokeColor: "white"
                        strokeWidth: 2
                        joinStyle: ShapePath.RoundJoin
                        startX: marker.width / 2
                        startY: 1
                        PathLine {
                            x: marker.width - 2
                            y: marker.height - 2
                        }
                        PathLine {
                            x: marker.width / 2
                            y: marker.height * 0.7
                        }
                        PathLine {
                            x: 2
                            y: marker.height - 2
                        }
                        PathLine {
                            x: marker.width / 2
                            y: 1
                        }
                    }
                }
            }

            Label {
                anchors.horizontalCenter: marker.horizontalCenter
                anchors.bottom: marker.top
                anchors.bottomMargin: 2
                // speed is reported in m/s
                text: memberItem.isMoving ? qsTr("%1 (%2 km/h)").arg(memberItem.history.name).arg(Math.round(memberItem.history.speed * 3.6)) : memberItem.history.name
                font.bold: true
                style: Text.Outline
                styleColor: "white"
            }
        }
    }

    Timer {
        id: groupPosTimer
        interval: 1000 * 10
        repeat: true
        onTriggered: session.requestGroupPositionsUpdate()
        // if we are a group share and active
        running: session.shareType !== QHaukSession.Solo && session.isSharing
        Component.onCompleted: {
            if (running) {
                session.requestGroupPositionsUpdate();
            }
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            ToolButton {
                text: "‹"
                icon.name: "go-previous"
                display: AbstractButton.IconOnly
                font.pixelSize: Qt.application.font.pixelSize * 1.6
                Layout.preferredWidth: page.touchTarget
                Layout.preferredHeight: page.touchTarget
                Accessible.name: qsTr("Back")
                onClicked: page.StackView.view.pop()
            }

            Label {
                text: page.title
                font.bold: true
                font.pixelSize: Qt.application.font.pixelSize * 1.2
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: true
            }
            Label {
                Layout.fillWidth: true
                Layout.preferredHeight: window.touchTarget + 4
                visible: session.error.length !== 0
                text: qsTr("An error occurred: '%1'").arg(session.error)
            }
        }
    }

    // No Flickable here: it would steal the pan/pinch gestures from the map
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // One button per group member, centers the map on their latest known position
        ListView {
            id: memberList
            Layout.fillWidth: true
            Layout.preferredHeight: page.touchTarget
            Layout.margins: 4
            visible: count > 0
            orientation: ListView.Horizontal
            spacing: 8
            clip: true
            model: page.session.isSharing ? page.session.groupPositions : []

            // The user is not part of groupPositions, resumes following the user's position
            header: Item {
                // spacing does not apply to the header
                width: userButton.width + memberList.spacing
                height: memberList.height

                Button {
                    id: userButton
                    readonly property QHaukPositionHistory history: page.session.positions

                    height: parent.height
                    text: history.name
                    enabled: history.positions.length > 0
                    palette.button: history.color
                    Accessible.name: qsTr("Center map on %1").arg(history.name)
                    onClicked: mapView.following = true
                }
            }
            headerPositioning: ListView.InlineHeader

            delegate: Button {
                required property QHaukPositionHistory modelData

                height: memberList.height
                text: modelData.name
                enabled: modelData.positions.length > 0
                palette.button: modelData.color
                Accessible.name: qsTr("Center map on %1").arg(modelData.name)
                onClicked: {
                    mapView.following = false;
                    mapView.centerOn(modelData.lastKnownPosition);
                }
            }
        }

        TouchMapView {
            id: mapView
            Layout.fillWidth: true
            Layout.fillHeight: true

            map.plugin: Plugin {
                name: "osm"
                // The osm plugin creates its own QNetworkAccessManagers and ignores the
                // engine's QQmlNetworkAccessManagerFactory, so QHaukNam can't set the user agent here.
                // Tile servers require an identifying user agent
                PluginParameter {
                    name: "osm.useragent"
                    value: "org.kde.korvus"
                }
                PluginParameter {
                    name: "osm.mapping.providersrepository.address"
                    value: "https://autoconfig.kde.org/qtlocation/"
                }
            }
            map.zoomLevel: 15
            followCoordinate: page.session.positions.lastKnownPosition

            // Trail of past positions per group member, the markers draw on top (z)
            MapItemView {
                parent: mapView.map
                model: page.session.isSharing ? page.session.groupPositions : []

                delegate: MemberTrail {
                    required property QHaukPositionHistory modelData
                    history: modelData
                }
            }

            // One marker per group member, placed at their latest known position
            MapItemView {
                parent: mapView.map
                model: page.session.isSharing ? page.session.groupPositions : []

                delegate: MemberMarker {
                    required property QHaukPositionHistory modelData
                    history: modelData
                    map: mapView.map
                }
            }

            // The user's own position is updated far more often than the group positions,
            // it is kept out of groupPositions so its updates don't recreate the group delegates
            MemberTrail {
                history: page.session.positions
                visible: page.session.isSharing && history.positions.length > 1
                Component.onCompleted: mapView.map.addMapItem(this)
            }

            MemberMarker {
                history: page.session.positions
                map: mapView.map
                visible: page.session.isSharing && history.positions.length > 0
                Component.onCompleted: mapView.map.addMapItem(this)
            }
        }

        Button {
            text: qsTr("Copy share information to clipboard")
            icon.name: "edit-copy"
            flat: true
            Layout.fillWidth: true
            Layout.preferredHeight: window.touchTarget
            onClicked: QHauk.setClipboard(session.shareType === QHaukSession.Solo ? qsTr("You can follow my location share %1").arg(session.url) : qsTr("You can join my location share %1 PIN: %2").arg(session.url).arg(session.pin))
        }
    }
}
