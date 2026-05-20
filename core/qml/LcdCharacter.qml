import QtQuick

Item {
    id: character
    property var pattern: []
    property color activeColor: "#b6ff65"
    property color inactiveColor: "#28472c"
    property color gridColor: "#4d7349"
    property bool showGrid: true
    property bool editable: false
    property int pixelSize: 10
    property int pixelSpacing: 2
    signal pixelClicked(int row, int column)
    signal pixelPainted(int row, int column, bool active)

    property int lastPaintIndex: -1
    property bool paintActive: true

    implicitWidth: 5 * pixelSize + 4 * pixelSpacing
    implicitHeight: 8 * pixelSize + 7 * pixelSpacing

    function pixelIndexAt(mouseX, mouseY) {
        var step = pixelSize + pixelSpacing
        var column = Math.floor(mouseX / step)
        var row = Math.floor(mouseY / step)
        if (row < 0 || row >= 8 || column < 0 || column >= 5)
            return -1
        if (mouseX - column * step > pixelSize || mouseY - row * step > pixelSize)
            return -1
        return row * 5 + column
    }

    function isPixelActive(index) {
        return pattern.length > index && pattern[index] === 1
    }

    function paintAt(mouseX, mouseY) {
        var index = pixelIndexAt(mouseX, mouseY)
        if (index < 0 || index === lastPaintIndex)
            return
        lastPaintIndex = index
        pixelPainted(Math.floor(index / 5), index % 5, paintActive)
    }

    Grid {
        id: pixelsGrid
        anchors.fill: parent
        columns: 5
        spacing: character.pixelSpacing

        Repeater {
            id: pixels
            model: 40
            Rectangle {
                required property int index
                width: character.pixelSize
                height: character.pixelSize
                color: isActive ? character.activeColor : character.inactiveColor
                border.width: 0
                border.color: character.gridColor
                property bool isActive: character.isPixelActive(index)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: character.editable
        hoverEnabled: enabled
        acceptedButtons: Qt.LeftButton
        preventStealing: true

        onPressed: (mouse) => {
            var index = character.pixelIndexAt(mouse.x, mouse.y)
            if (index < 0)
                return
            character.lastPaintIndex = -1
            character.paintActive = !character.isPixelActive(index)
            character.pixelClicked(Math.floor(index / 5), index % 5)
            character.paintAt(mouse.x, mouse.y)
        }

        onPositionChanged: (mouse) => {
            if (pressed)
                character.paintAt(mouse.x, mouse.y)
        }

        onReleased: {
            character.lastPaintIndex = -1
        }
    }
}
