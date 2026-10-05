// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.hauk 1.0
import org.kde.hauk.app

Page {
    id: page
    title: qsTr("Settings")

    readonly property int touchTarget: 48
    readonly property int pageMargin: 16
    readonly property int maxContentWidth: 600

    function save() {
        AppSettings.host = hostField.text;
        AppSettings.password = passwordField.text;
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            ToolButton {
                text: "‹"
                display: AbstractButton.IconOnly
                font.pixelSize: Qt.application.font.pixelSize * 1.6
                Layout.preferredWidth: page.touchTarget
                Layout.preferredHeight: page.touchTarget
                Accessible.name: qsTr("Back")
                onClicked: {
                    page.save();
                    mainView.pop();
                }
            }

            Label {
                text: page.title
                font.bold: true
                font.pixelSize: Qt.application.font.pixelSize * 1.2
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: true
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: width
        contentHeight: form.implicitHeight + 2 * page.pageMargin
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollIndicator.vertical: ScrollIndicator {}

        ColumnLayout {
            id: form
            width: Math.min(flickable.width - 2 * page.pageMargin, page.maxContentWidth)
            x: (flickable.width - width) / 2
            y: page.pageMargin
            spacing: 6

            // Labels above fields so the fields get the full screen width
            Label {
                text: qsTr("Host")
                font.bold: true
            }

            TextField {
                id: hostField
                Layout.fillWidth: true
                Layout.preferredHeight: page.touchTarget
                text: AppSettings.host
                placeholderText: "https://hauk.live/"
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                onEditingFinished: AppSettings.host = text
                onAccepted: passwordField.forceActiveFocus()
            }

            Label {
                text: qsTr("Password")
                font.bold: true
                Layout.topMargin: 12
            }

            TextField {
                id: passwordField
                Layout.fillWidth: true
                Layout.preferredHeight: page.touchTarget
                text: AppSettings.password
                echoMode: TextInput.PasswordEchoOnEdit
                inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhHiddenText
                onEditingFinished: AppSettings.password = text
                onAccepted: mapApiKeyField.forceActiveFocus()
            }

            Button {
                text: qsTr("Copy log to clipboard")
                onClicked: QHauk.copyLogToClipboard()
                icon.name: "edit-copy"
            }

            Button {
                text: qsTr("Done")
                highlighted: true
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.preferredHeight: page.touchTarget
                onClicked: {
                    page.save();
                    mainView.pop();
                }
            }
        }
    }
}
