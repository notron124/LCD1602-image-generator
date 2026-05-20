import QtQuick

Rectangle {
    id: lcd1602
    property int revision: 0
    property int pixelSize: 6
    property int pixelSpacing: 1
    property int selectedRow: 0
    property int selectedColumn: 0
    property bool cursorVisible: true
    property color backgroundColor: lcdBackend.backgroundColor
    property color pixelColor: lcdBackend.pixelColor
    property color backlightColor: lcdBackend.backlightColor
    property bool showGrid: lcdBackend.showGrid
    property bool deviceFrame: lcdBackend.deviceFrame
    signal cellClicked(int row, int column)
    signal textEntered(string text)
    signal backspacePressed()
    signal deletePressed()
    signal moveRequested(int rowDelta, int columnDelta)

    implicitWidth: frame.width
    implicitHeight: frame.height
    width: implicitWidth
    height: implicitHeight
    color: "transparent"
    focus: true
    activeFocusOnTab: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Backspace) {
            lcd1602.backspacePressed()
            event.accepted = true
        } else if (event.key === Qt.Key_Delete) {
            lcd1602.deletePressed()
            event.accepted = true
        } else if (event.key === Qt.Key_Left) {
            lcd1602.moveRequested(0, -1)
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            lcd1602.moveRequested(0, 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Up) {
            lcd1602.moveRequested(-1, 0)
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            lcd1602.moveRequested(1, 0)
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            lcd1602.moveRequested(1, 0)
            event.accepted = true
        } else if (event.text.length > 0 && event.text >= " ") {
            lcd1602.textEntered(event.text)
            event.accepted = true
        }
    }

    Connections {
        target: lcdBackend
        function onDisplayChanged() {
            lcd1602.revision += 1
        }
    }

    Timer {
        interval: 520
        running: true
        repeat: true
        onTriggered: lcd1602.cursorVisible = !lcd1602.cursorVisible
    }

    Rectangle {
        id: frame
        width: screen.width + (lcd1602.deviceFrame ? 28 : 0)
        height: screen.height + (lcd1602.deviceFrame ? 28 : 0)
        anchors.centerIn: parent
        radius: 6
        color: lcd1602.deviceFrame ? "#252b2f" : "transparent"
        border.width: lcd1602.deviceFrame ? 2 : 0
        border.color: "#0f1113"

        Rectangle {
            id: screen
            width: charGrid.width + 24
            height: charGrid.height + 24
            anchors.centerIn: parent
            radius: 3
            color: lcd1602.backlightColor
            border.width: 3
            border.color: Qt.darker(lcd1602.backgroundColor, 1.3)

            Rectangle {
                anchors.fill: parent
                anchors.margins: 6
                color: lcd1602.backgroundColor
                opacity: 0.86
            }

            Grid {
                id: charGrid
                anchors.centerIn: parent
                columns: 16
                rowSpacing: 8
                columnSpacing: 4

                Repeater {
                    id: characters
                    model: 32

                    Item {
                        id: cell
                        required property int index
                        property int row: Math.floor(index / 16)
                        property int column: index % 16
                        property bool selected: row === lcd1602.selectedRow && column === lcd1602.selectedColumn
                        width: characterItem.width
                        height: characterItem.height

                        LcdCharacter {
                            id: characterItem
                            pixelSize: lcd1602.pixelSize
                            pixelSpacing: lcd1602.pixelSpacing
                            activeColor: lcd1602.pixelColor
                            inactiveColor: Qt.rgba(lcd1602.pixelColor.r, lcd1602.pixelColor.g, lcd1602.pixelColor.b, 0.16)
                            gridColor: Qt.rgba(lcd1602.pixelColor.r, lcd1602.pixelColor.g, lcd1602.pixelColor.b, 0.28)
                            showGrid: lcd1602.showGrid
                            pattern: lcdBackend.cellPattern(cell.index, lcd1602.revision)
                        }

                        Item {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            width: characterItem.width
                            height: lcd1602.pixelSize
                            opacity: cell.selected && lcd1602.cursorVisible ? 1 : 0

                            Repeater {
                                model: 5

                                Rectangle {
                                    required property int index
                                    x: index * (lcd1602.pixelSize + lcd1602.pixelSpacing)
                                    width: lcd1602.pixelSize
                                    height: lcd1602.pixelSize
                                    color: lcd1602.pixelColor
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: lcd1602.cellClicked(cell.row, cell.column)
                        }
                    }
                }
            }
        }
    }
}
