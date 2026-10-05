// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtLocation
import QtPositioning

import org.kde.hauk 1.0
import org.kde.hauk.app

Page {
    id: page
    title: session.isSharing ? session.shareType === QHaukSession.Solo ? qsTr("ShareID: %1").arg(session.shareId) : qsTr("ShareID: %1 PIN: %2").arg(session.shareId).arg(session.pin) : qsTr("The session has ended")

    required property QHaukSession session
    readonly property int touchTarget: 48
    readonly property int pageMargin: 16
    readonly property int maxContentWidth: 600

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

            delegate: Button {
                required property qHaukPositionHistory modelData

                height: memberList.height
                text: modelData.name
                enabled: modelData.positions.length > 0
                palette.button: modelData.color
                Accessible.name: qsTr("Center map on %1").arg(modelData.name)
                onClicked: {
                    mapView.following = false;
                    mapView.centerOn(modelData.positions[modelData.positions.length - 1].position);
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
                    value: "org.kde.qhauk"
                }
                PluginParameter {
                    name: "osm.mapping.providersrepository.address"
                    value: "https://autoconfig.kde.org/qtlocation/"
                }
            }
            map.zoomLevel: 15
            followCoordinate: page.session.position.position

            // Trail of past positions per group member, declared first so the markers draw on top
            MapItemView {
                parent: mapView.map
                model: page.session.isSharing ? page.session.groupPositions : []

                delegate: MapPolyline {
                    required property qHaukPositionHistory modelData

                    visible: modelData.positions.length > 1
                    path: modelData.positions.map(p => p.position)
                    line.width: 3
                    line.color: modelData.color
                    opacity: 0.7
                }
            }

            // One marker per group member, placed at their latest known position
            MapItemView {
                parent: mapView.map
                model: page.session.isSharing ? page.session.groupPositions : []

                delegate: MapQuickItem {
                    id: memberItem
                    required property qHaukPositionHistory modelData

                    readonly property var lastPosition: modelData.positions.length > 0 ? modelData.positions[modelData.positions.length - 1] : undefined
                    readonly property bool isMoving: !!lastPosition && lastPosition.speed > 0
                    // Direction of travel in degrees from north, derived from the last two recorded points
                    readonly property real heading: modelData.positions.length > 1 ? modelData.positions[modelData.positions.length - 2].position.azimuthTo(modelData.positions[modelData.positions.length - 1].position) : 0
                    // Accuracy (meters) converted to screen pixels, re-evaluated whenever the zoom changes
                    readonly property real accuracyRadius: {
                        mapView.map.zoomLevel;
                        if (!lastPosition || !(lastPosition.accuracy > 0)) {
                            return 0;
                        }
                        const center = mapView.map.fromCoordinate(lastPosition.position, false);
                        const edge = mapView.map.fromCoordinate(lastPosition.position.atDistanceAndAzimuth(lastPosition.accuracy, 90), false);
                        return Math.hypot(edge.x - center.x, edge.y - center.y);
                    }

                    visible: modelData.positions.length > 0
                    coordinate: lastPosition ? lastPosition.position : QtPositioning.coordinate()
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
                            color: Qt.rgba(memberItem.modelData.color.r, memberItem.modelData.color.g, memberItem.modelData.color.b, 0.2)
                            border.color: Qt.rgba(memberItem.modelData.color.r, memberItem.modelData.color.g, memberItem.modelData.color.b, 0.5)
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
                                color: memberItem.modelData.color
                                border.color: "white"
                                border.width: 2
                            }

                            // Arrow pointing up (north), rotated to the heading relative to the map bearing
                            Shape {
                                anchors.fill: parent
                                visible: memberItem.isMoving
                                rotation: memberItem.heading - mapView.map.bearing
                                preferredRendererType: Shape.CurveRenderer

                                ShapePath {
                                    fillColor: memberItem.modelData.color
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
                            text: memberItem.isMoving ? qsTr("%1 (%2 km/h)").arg(memberItem.modelData.name).arg(Math.round(memberItem.lastPosition.speed * 3.6)) : memberItem.modelData.name
                            font.bold: true
                            style: Text.Outline
                            styleColor: "white"
                        }
                    }
                }
            }
        }

        Button {
            enabled: page.session.isSharing
            Layout.fillWidth: true
            Layout.maximumWidth: page.maxContentWidth
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: page.pageMargin
            Layout.preferredHeight: page.touchTarget + 4
            text: qsTr("Stop Sharing")
            icon.name: "media-playback-stop"
            font.bold: true
            onClicked: {
                page.session.stopSharing();
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            enabled: session.isSharing
            spacing: 12
            Button {
                text: qsTr("🌐Open in Browser")
                //icon: ""
                flat: true
                Layout.fillWidth: true
                Layout.preferredHeight: window.touchTarget
                onClicked: Qt.openUrlExternally(session.url)
            }

            Button {
                text: qsTr("Copy share id to clipboard")
                icon.name: "edit-copy"
                flat: true
                Layout.fillWidth: true
                Layout.preferredHeight: window.touchTarget
                onClicked: QHauk.setClipboard(session.shareType === QHaukSession.Solo ? session.shareId : qsTr("You can join my location share %1 PIN: %2").arg(session.shareId).arg(session.pin))
            }
        }
    }
}
