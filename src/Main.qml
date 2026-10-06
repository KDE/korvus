// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import QtPositioning

import org.kde.hauk 1.0
import org.kde.hauk.app

ApplicationWindow {
    id: window
    // Only used on desktop; on mobile the window is always full screen.
    width: 390
    height: 844
    visible: true
    title: qsTr("QHauk")

    // Minimum touch target size recommended by Apple HIG / Material guidelines
    readonly property int touchTarget: 48
    readonly property int pageMargin: 16
    // Keep content readable on tablets / desktop
    readonly property int maxContentWidth: 600

    property QHaukSession session: QHauk.createSession()
    property list<QHaukSession> followSessions

    Binding {
        target: QHauk
        property: "host"
        value: AppSettings.host
    }
    Binding {
        target: QHauk
        property: "password"
        value: AppSettings.password
    }
    Binding {
        target: QHauk
        property: "nickName"
        value: AppSettings.nickName
    }
    Binding {
        target: session
        property: "shareType"
        value: AppSettings.shareType
    }
    Binding {
        target: session
        property: "duration"
        value: AppSettings.duration
    }

    Connections {
        target: session
        function onIsSharingChanged() {
            if (session.isSharing) {
                mainView.push("HaukMap.qml", {
                    "session": session
                });
            }
        }
    }

    StackView {
        id: mainView
        anchors.fill: parent
        initialItem: haukView
    }

    Component {
        id: haukView

        Page {
            title: qsTr("QHauk")

            header: ToolBar {
                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Label {
                        text: qsTr("QHauk")
                        font.bold: true
                        font.pixelSize: Qt.application.font.pixelSize * 1.2
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.leftMargin: window.pageMargin
                        verticalAlignment: Text.AlignVCenter
                    }

                    ToolButton {
                        text: "⋮"
                        font.bold: true
                        font.pixelSize: Qt.application.font.pixelSize * 1.6
                        Layout.preferredWidth: window.touchTarget
                        Layout.preferredHeight: window.touchTarget
                        Accessible.name: qsTr("Settings")
                        onClicked: mainView.push("SettingsView.qml")
                    }
                }
            }

            Flickable {
                id: flickable
                anchors.fill: parent
                contentWidth: width
                contentHeight: content.implicitHeight + 2 * window.pageMargin
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                ScrollIndicator.vertical: ScrollIndicator {}

                ColumnLayout {
                    id: content
                    width: Math.min(flickable.width - 2 * window.pageMargin, window.maxContentWidth)
                    x: (flickable.width - width) / 2
                    y: window.pageMargin
                    spacing: 16

                    Label {
                        text: session.isSharing ? qsTr("Sharing Active") : qsTr("Sharing Inactive")
                        font.pixelSize: Qt.application.font.pixelSize * 1.4
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                    }

                    Label {
                        text: AppSettings.host.toString()
                        visible: text.length > 0
                        opacity: 0.7
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        enabled: !session.isSharing
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 12

                        Label {
                            text: qsTr("Nickname")
                        }
                        TextField {
                            Layout.fillWidth: true
                            Layout.preferredHeight: window.touchTarget
                            verticalAlignment: Text.AlignVCenter
                            text: QHauk.nickName
                            onEditingFinished: AppSettings.nickName = text
                        }

                        Label {
                            text: qsTr("Share type")
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            Layout.preferredHeight: window.touchTarget
                            model: [qsTr("Solo share"), qsTr("Groupshare"), qsTr("Join groupshare")]
                            currentIndex: AppSettings.shareType
                            onActivated: index => AppSettings.shareType = index
                        }

                        Label {
                            text: qsTr("Duration")
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            SpinBox {
                                id: shareDuration
                                Layout.fillWidth: true
                                Layout.preferredHeight: window.touchTarget
                                from: 1
                                to: 999
                                editable: true
                                inputMethodHints: Qt.ImhDigitsOnly
                                // the duration is in minutes, the spin box shows it in the selected unit
                                onValueModified: AppSettings.duration = value * unit.currentValue
                            }

                            ComboBox {
                                id: unit
                                Layout.preferredHeight: window.touchTarget
                                implicitContentWidthPolicy: ComboBox.WidestText
                                // the style's background enforces a large implicit width, size to the content instead
                                Layout.preferredWidth: implicitContentWidth + leftPadding + rightPadding
                                model: ListModel {
                                    ListElement {
                                        text: qsTr("minutes")
                                        factor: 1
                                    }
                                    ListElement {
                                        text: qsTr("hours")
                                        factor: 60
                                    }
                                }
                                textRole: "text"
                                valueRole: "factor"
                                onActivated: AppSettings.duration = shareDuration.value * currentValue
                                Component.onCompleted: {
                                    currentIndex = AppSettings.duration % 60 === 0 ? 1 : 0;
                                    shareDuration.value = AppSettings.duration / valueAt(currentIndex);
                                }
                            }
                        }

                        Label {
                            text: qsTr("Pin")
                            visible: AppSettings.shareType === QHaukSession.JoinGroup
                        }
                        TextField {
                            visible: AppSettings.shareType === QHaukSession.JoinGroup
                            Layout.fillWidth: true
                            Layout.preferredHeight: window.touchTarget
                            verticalAlignment: Text.AlignVCenter
                            text: session.pin
                            onTextEdited: session.pin = acceptableInput ? text : ""
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: RegularExpressionValidator {
                                regularExpression: /[0-9]{6}/
                            }
                            maximumLength: 6
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: window.touchTarget + 4
                        enabled: AppSettings.shareType !== QHaukSession.JoinGroup || session.pin.length === 6
                        text: session.isSharing ? qsTr("Stop Sharing") : qsTr("Start Sharing")
                        icon.name: session.isSharing ? "media-playback-stop" : "media-playback-start"
                        font.bold: true
                        highlighted: !session.isSharing
                        onClicked: {
                            if (session.isSharing) {
                                session.stop();
                            } else {
                                session.startSharing();
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: window.touchTarget
                        visible: session.isSharing
                        text: qsTr("Show Map")
                        icon.name: "map-flat"
                        onClicked: mainView.push("HaukMap.qml", {
                            "session": session
                        })
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: session.error.length !== 0
                        text: qsTr("An error occurred: '%1'").arg(session.error)
                        color: "red"
                        wrapMode: Text.Wrap
                    }

                    MenuSeparator {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Following")
                        font.bold: true
                        font.pixelSize: Qt.application.font.pixelSize * 1.2
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: window.followSessions.length === 0
                        text: qsTr("You are not following anyone")
                        opacity: 0.7
                        wrapMode: Text.Wrap
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: window.followSessions.length !== 0
                        spacing: 0

                        Repeater {
                            id: followerButtons
                            model: window.followSessions
                            delegate: ItemDelegate {
                                id: followDelegate
                                required property QHaukSession modelData
                                TextMetrics {
                                    id: elidedGroupName
                                    text: modelData.groupNames.join(", ")
                                    elideWidth: followDelegate.width - stopFollowing.width - 50
                                    elide: Qt.ElideRight
                                }
                                Layout.fillWidth: true
                                Layout.preferredHeight: window.touchTarget
                                text: qsTr("%1 (%2)").arg(modelData.shareId).arg(elidedGroupName.elidedText)
                                font.bold: true
                                icon.name: "mark-location"
                                rightPadding: stopFollowing.width
                                onClicked: mainView.push("HaukMap.qml", {
                                    "session": modelData
                                })

                                ToolButton {
                                    id: stopFollowing
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: window.touchTarget
                                    height: window.touchTarget
                                    text: "✕"
                                    Accessible.name: qsTr("Stop following %1").arg(followDelegate.modelData.shareId)
                                    onClicked: followDelegate.modelData.stop()
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: window.touchTarget
                        text: qsTr("Follow others")
                        icon.name: "list-add"
                        onClicked: {
                            followIdField.clear();
                            followDialog.open();
                        }
                    }
                }
            }

            Dialog {
                id: followDialog
                parent: Overlay.overlay
                anchors.centerIn: parent
                width: Math.min(window.width - 2 * window.pageMargin, window.maxContentWidth)
                modal: true
                title: qsTr("Follow others")
                standardButtons: Dialog.Ok | Dialog.Cancel

                // only allow accepting a complete share id
                Component.onCompleted: standardButton(Dialog.Ok).enabled = Qt.binding(() => followIdField.acceptableInput)
                onOpened: followIdField.forceActiveFocus()

                onAccepted: {
                    const shareId = followIdField.text.toUpperCase();
                    const followSession = QHauk.createSession();
                    followSession.startFollowing(shareId);
                    // drop the session from the list once it was stopped
                    followSession.isSharingChanged.connect(() => {
                        if (!followSession.isSharing) {
                            window.followSessions = Array.from(window.followSessions).filter(s => s !== followSession);
                        }
                    });
                    window.followSessions.push(followSession);
                    mainView.push("HaukMap.qml", {
                        "session": followSession
                    });
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: qsTr("Share ID")
                    }
                    TextField {
                        id: followIdField
                        Layout.fillWidth: true
                        Layout.preferredHeight: window.touchTarget
                        verticalAlignment: Text.AlignVCenter
                        placeholderText: "31BN-MMCB"
                        inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText
                        validator: RegularExpressionValidator {
                            regularExpression: /[A-Za-z0-9]{4}-[A-Za-z0-9]{4}/
                        }
                        maximumLength: 9
                        onAccepted: {
                            if (acceptableInput) {
                                followDialog.accept();
                            }
                        }
                    }
                }
            }
        }
    }
}
