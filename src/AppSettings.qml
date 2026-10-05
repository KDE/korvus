pragma Singleton

import QtCore

Settings {
    property string nickName: "Me"
    property int shareType: 0 // solo share
    property int duration: 60 // minutes

    property url host: "https://hauk.live/"
    // TODO: QtKeychain
    property string password: ""
}
