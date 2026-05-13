import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1220
    height: 760
    minimumWidth: 920
    minimumHeight: 620
    visible: true
    title: qsTr("Генератор LCD1602")
    color: "#f0f2f5"

    property int customRevision: 0
    property int fontRevision: 0
    property int selectedCustomChar: 0
    property int selectedRow: 0
    property int selectedColumn: 0
    property string selectedFontChar: "A"

    Connections {
        target: lcdBackend
        function onCustomCharsChanged() { root.customRevision += 1 }
        function onFontChanged() { root.fontRevision += 1 }
    }

    FileDialog {
        id: saveProjectDialog
        title: "Сохранить проект"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Проект LCD1602 (*.json)", "JSON (*.json)"]
        defaultSuffix: "json"
        onAccepted: lcdBackend.saveProject(selectedFile)
    }

    FileDialog {
        id: loadProjectDialog
        title: "Загрузить проект"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Проект LCD1602 (*.json)", "JSON (*.json)"]
        onAccepted: lcdBackend.loadProject(selectedFile)
    }

    FileDialog {
        id: exportDialog
        title: "Сохранить изображение"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG (*.png)"]
        defaultSuffix: "png"
        onAccepted: lcdBackend.exportPng(selectedFile)
    }

    FileDialog {
        id: saveFontDialog
        title: "Сохранить весь шрифт"
        fileMode: FileDialog.SaveFile
        nameFilters: ["BDF шрифт (*.bdf)", "Все файлы (*)"]
        defaultSuffix: "bdf"
        onAccepted: lcdBackend.saveFont(selectedFile)
    }

    FileDialog {
        id: loadFontDialog
        title: "Загрузить шрифт"
        fileMode: FileDialog.OpenFile
        nameFilters: ["BDF шрифт (*.bdf)", "JSON шрифт (*.json)", "Все файлы (*)"]
        onAccepted: lcdBackend.loadFont(selectedFile)
    }

    Shortcut { sequence: "Ctrl+S"; onActivated: saveProjectDialog.open() }
    Shortcut { sequence: "Ctrl+O"; onActivated: loadProjectDialog.open() }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 8

            Label {
                text: "Генератор LCD1602"
                font.pixelSize: 17
                font.bold: true
                Layout.fillWidth: true
            }

            Button { text: "PNG"; onClicked: exportDialog.open() }
            Button { text: "Сохранить проект"; onClicked: saveProjectDialog.open() }
            Button { text: "Загрузить проект"; onClicked: loadProjectDialog.open() }
        }
    }

    footer: Label {
        text: lcdBackend.status
        padding: 8
        color: "#4a5560"
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            Pane {
                id: displayPane
                Layout.fillWidth: true
                Layout.margins: 18
                padding: 18

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 14

                    Lcd1602 {
                        id: display
                        property int fittedPixelSize: Math.max(3, Math.min(10, Math.floor((displayPane.width - 120) / 116)))
                        pixelSize: fittedPixelSize
                        pixelSpacing: Math.max(1, Math.floor(fittedPixelSize / 4))
                        selectedRow: root.selectedRow
                        selectedColumn: root.selectedColumn
                        Layout.alignment: Qt.AlignHCenter
                        onCellClicked: (row, column) => {
                            root.selectedRow = row
                            root.selectedColumn = column
                            if (row === 0) {
                                line1Field.forceActiveFocus()
                                line1Field.cursorPosition = column
                            } else {
                                line2Field.forceActiveFocus()
                                line2Field.cursorPosition = column
                            }
                        }
                    }

                    GridLayout {
                        columns: 4
                        Layout.fillWidth: true
                        columnSpacing: 10

                        Label { text: "Строка 1" }
                        TextField {
                            id: line1Field
                            Layout.fillWidth: true
                            maximumLength: 16
                            text: lcdBackend.line1
                            onTextEdited: lcdBackend.line1 = text
                            onCursorPositionChanged: {
                                root.selectedRow = 0
                                root.selectedColumn = Math.min(cursorPosition, 15)
                            }
                        }

                        Label { text: "Строка 2" }
                        TextField {
                            id: line2Field
                            Layout.fillWidth: true
                            maximumLength: 16
                            text: lcdBackend.line2
                            onTextEdited: lcdBackend.line2 = text
                            onCursorPositionChanged: {
                                root.selectedRow = 1
                                root.selectedColumn = Math.min(cursorPosition, 15)
                            }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width < 1050 ? 1 : 2
                columnSpacing: 18
                rowSpacing: 18

                Pane {
                    Layout.fillWidth: true
                    Layout.margins: 18
                    padding: 18

                    GridLayout {
                        anchors.fill: parent
                        columns: 4
                        rowSpacing: 10
                        columnSpacing: 10

                        Label { text: "Настройки"; Layout.columnSpan: 4; font.bold: true }
                        Button { text: "Зеленая"; onClicked: lcdBackend.applyTheme("green") }
                        Button { text: "Моно"; onClicked: lcdBackend.applyTheme("mono") }
                        Button { text: "Синяя"; onClicked: lcdBackend.applyTheme("blue") }
                        Item { Layout.fillWidth: true }

                        Label { text: "Фон" }
                        TextField {
                            Layout.fillWidth: true
                            text: lcdBackend.backgroundColor
                            onEditingFinished: lcdBackend.backgroundColor = text
                        }
                        Label { text: "Пиксели" }
                        TextField {
                            Layout.fillWidth: true
                            text: lcdBackend.pixelColor
                            onEditingFinished: lcdBackend.pixelColor = text
                        }

                        Label { text: "Подсветка" }
                        TextField {
                            Layout.fillWidth: true
                            text: lcdBackend.backlightColor
                            onEditingFinished: lcdBackend.backlightColor = text
                        }
                        Label { text: "Масштаб PNG" }
                        ComboBox {
                            model: [1, 2, 4, 8]
                            currentIndex: model.indexOf(lcdBackend.exportScale)
                            onActivated: (index) => lcdBackend.exportScale = model[index]
                        }

                        CheckBox {
                            text: "Сетка"
                            checked: lcdBackend.showGrid
                            onToggled: lcdBackend.showGrid = checked
                        }
                        CheckBox {
                            text: "Рамка устройства"
                            checked: lcdBackend.deviceFrame
                            onToggled: lcdBackend.deviceFrame = checked
                        }
                        Button { text: "Экспорт PNG"; onClicked: exportDialog.open() }
                        Item { Layout.fillWidth: true }
                    }
                }

                Pane {
                    Layout.fillWidth: true
                    Layout.margins: 18
                    padding: 18

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label { text: "Кастомные символы CGRAM"; font.bold: true }

                        RowLayout {
                            spacing: 8
                            Repeater {
                                model: 8
                                Button {
                                    required property int index
                                    text: String(index)
                                    checkable: true
                                    checked: root.selectedCustomChar === index
                                    onClicked: root.selectedCustomChar = index
                                }
                            }
                        }

                        RowLayout {
                            spacing: 18

                            LcdCharacter {
                                pixelSize: 20
                                pixelSpacing: 3
                                editable: true
                                activeColor: lcdBackend.pixelColor
                                inactiveColor: "#e3e8df"
                                gridColor: "#9ba899"
                                pattern: lcdBackend.customCharPattern(root.selectedCustomChar, root.customRevision)
                                onPixelPainted: (row, column, active) => lcdBackend.setCustomPixel(root.selectedCustomChar, row, column, active)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Label { text: "Вставка на дисплей" }
                                RowLayout {
                                    SpinBox { id: insertRow; from: 1; to: 2; value: root.selectedRow + 1 }
                                    SpinBox { id: insertColumn; from: 1; to: 16; value: root.selectedColumn + 1 }
                                }
                                Button {
                                    text: "Вставить символ"
                                    onClicked: lcdBackend.insertCustomChar(root.selectedCustomChar, insertRow.value - 1, insertColumn.value - 1)
                                }
                                Button {
                                    text: "Очистить символ"
                                    onClicked: lcdBackend.clearCustomChar(root.selectedCustomChar)
                                }
                            }
                        }
                    }
                }

                Pane {
                    Layout.fillWidth: true
                    Layout.columnSpan: root.width < 1050 ? 1 : 2
                    Layout.margins: 18
                    padding: 18

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "Редактор шрифта 5x8"
                                font.bold: true
                                Layout.fillWidth: true
                            }
                            Button { text: "Загрузить BDF"; onClicked: loadFontDialog.open() }
                            Button { text: "Сохранить весь шрифт"; onClicked: saveFontDialog.open() }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: "Добавить символы" }
                            TextField {
                                id: addFontCharsField
                                Layout.fillWidth: true
                                placeholderText: "Например: ABCxyz0123+-"
                                onAccepted: {
                                    lcdBackend.ensureFontCharacters(text)
                                    if (text.length > 0)
                                        root.selectedFontChar = text[0]
                                    text = ""
                                }
                            }
                            Button {
                                text: "Добавить"
                                onClicked: {
                                    lcdBackend.ensureFontCharacters(addFontCharsField.text)
                                    if (addFontCharsField.text.length > 0)
                                        root.selectedFontChar = addFontCharsField.text[0]
                                    addFontCharsField.text = ""
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: lcdBackend.fontCharacters.length

                                Button {
                                    required property int index
                                    width: 38
                                    height: 34
                                    text: lcdBackend.fontCharacters.charAt(index) === " " ? "SP" : lcdBackend.fontCharacters.charAt(index)
                                    checkable: true
                                    checked: root.selectedFontChar === lcdBackend.fontCharacters.charAt(index)
                                    onClicked: root.selectedFontChar = lcdBackend.fontCharacters.charAt(index)
                                }
                            }
                        }

                        RowLayout {
                            spacing: 18

                            LcdCharacter {
                                pixelSize: 22
                                pixelSpacing: 3
                                editable: true
                                activeColor: "#17202a"
                                inactiveColor: "#e8ebef"
                                gridColor: "#aab2bd"
                                pattern: lcdBackend.fontCharPattern(root.selectedFontChar, root.fontRevision)
                                onPixelPainted: (row, column, active) => lcdBackend.setFontPixel(root.selectedFontChar, row, column, active)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: "Редактируется символ: " + (root.selectedFontChar === " " ? "пробел" : root.selectedFontChar)
                                    font.bold: true
                                }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    color: "#4a5560"
                                    text: "Выберите символ в списке или добавьте сразу несколько символов через поле выше. Все изменения остаются в текущем шрифте и попадут в BDF при сохранении всего шрифта."
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
