import QtQuick 2.0

Item {
    property string title: "-- no title --"
    property string subtitle

    property string actionTitle
    property bool actionEnabled: true
    property bool activity

    property bool spacing: true

    y: spacing ? units.gu(100) : 0
    width: parent.width
    height: parent.height-y
}