#include "LcdBackend.h"

#include <algorithm>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QRegularExpression>
#include <QTextStream>
#include <QtMath>

namespace {
LcdBackend::Matrix makePattern(std::initializer_list<const char *> rows)
{
    LcdBackend::Matrix matrix;
    for (const char *row : rows) {
        QVector<int> pixels;
        for (int column = 0; column < 5; ++column)
            pixels.append(row[column] == '1' ? 1 : 0);
        matrix.append(pixels);
    }
    return matrix;
}
}

LcdBackend::LcdBackend(QObject *parent)
    : QObject(parent),
      m_line1("LCD1602"),
      m_line2("GENERATOR")
{
    m_customChars.resize(CustomCharCount);
    for (auto &matrix : m_customChars)
        matrix = blankMatrix();

    m_customChars[0] = makePattern({"00001", "00010", "00100", "01000", "10000", "00000", "00000", "00000"});
    m_customChars[1] = makePattern({"00100", "01110", "10101", "00100", "00100", "00100", "00100", "00000"});
    initializeFont();
}

QString LcdBackend::line1() const { return m_line1; }
QString LcdBackend::line2() const { return m_line2; }
QString LcdBackend::backgroundColor() const { return m_backgroundColor; }
QString LcdBackend::pixelColor() const { return m_pixelColor; }
QString LcdBackend::backlightColor() const { return m_backlightColor; }
bool LcdBackend::showGrid() const { return m_showGrid; }
bool LcdBackend::deviceFrame() const { return m_deviceFrame; }
int LcdBackend::exportScale() const { return m_exportScale; }
QString LcdBackend::status() const { return m_status; }

QString LcdBackend::fontCharacters() const
{
    QList<QChar> keys = m_font.keys();
    std::sort(keys.begin(), keys.end(), [](QChar left, QChar right) {
        return left.unicode() < right.unicode();
    });

    QString result;
    for (QChar key : keys) {
        if (!key.isPrint())
            continue;
        result.append(key);
    }
    return result;
}

void LcdBackend::setLine1(const QString &value)
{
    const QString normalized = normalizedLine(value);
    if (m_line1 == normalized)
        return;
    m_line1 = normalized;
    emit displayChanged();
}

void LcdBackend::setLine2(const QString &value)
{
    const QString normalized = normalizedLine(value);
    if (m_line2 == normalized)
        return;
    m_line2 = normalized;
    emit displayChanged();
}

void LcdBackend::setBackgroundColor(const QString &value)
{
    if (m_backgroundColor == value)
        return;
    m_backgroundColor = value;
    emit displayChanged();
}

void LcdBackend::setPixelColor(const QString &value)
{
    if (m_pixelColor == value)
        return;
    m_pixelColor = value;
    emit displayChanged();
}

void LcdBackend::setBacklightColor(const QString &value)
{
    if (m_backlightColor == value)
        return;
    m_backlightColor = value;
    emit displayChanged();
}

void LcdBackend::setShowGrid(bool value)
{
    if (m_showGrid == value)
        return;
    m_showGrid = value;
    emit displayChanged();
}

void LcdBackend::setDeviceFrame(bool value)
{
    if (m_deviceFrame == value)
        return;
    m_deviceFrame = value;
    emit displayChanged();
}

void LcdBackend::setExportScale(int value)
{
    const int clamped = qBound(1, value, 8);
    if (m_exportScale == clamped)
        return;
    m_exportScale = clamped;
    emit displayChanged();
}

void LcdBackend::applyTheme(const QString &themeId)
{
    if (themeId == "mono") {
        m_backgroundColor = "#f5f5f5";
        m_backlightColor = "#d8d8d8";
        m_pixelColor = "#111111";
    } else if (themeId == "blue") {
        m_backgroundColor = "#07254f";
        m_backlightColor = "#2567bf";
        m_pixelColor = "#d8f0ff";
    } else {
        m_backgroundColor = "#19381f";
        m_backlightColor = "#6ba842";
        m_pixelColor = "#b6ff65";
    }
    emit displayChanged();
}

QVariantList LcdBackend::cellPattern(int cellIndex, int) const
{
    return matrixToVariant(matrixForCell(cellIndex));
}

QVariantList LcdBackend::customCharPattern(int index, int) const
{
    if (index < 0 || index >= m_customChars.size())
        return matrixToVariant(blankMatrix());
    return matrixToVariant(m_customChars[index]);
}

QVariantList LcdBackend::fontCharPattern(const QString &character, int) const
{
    if (character.isEmpty())
        return matrixToVariant(blankMatrix());
    return matrixToVariant(matrixForCharacter(character.front()));
}

void LcdBackend::ensureFontCharacters(const QString &characters)
{
    bool changed = false;
    for (QChar character : characters) {
        if (character.isSpace() && character != ' ')
            continue;
        const QChar key = character;
        if (m_font.contains(key))
            continue;
        m_font.insert(key, blankMatrix());
        changed = true;
    }

    if (!changed)
        return;

    setStatus("Символы добавлены в шрифт");
    emit fontChanged();
    emit displayChanged();
}

void LcdBackend::toggleCustomPixel(int index, int row, int column)
{
    if (index < 0 || index >= m_customChars.size() || row < 0 || row >= CharHeight || column < 0 || column >= CharWidth)
        return;
    setCustomPixel(index, row, column, !m_customChars[index][row][column]);
}

void LcdBackend::toggleFontPixel(const QString &character, int row, int column)
{
    if (character.isEmpty() || row < 0 || row >= CharHeight || column < 0 || column >= CharWidth)
        return;
    const QChar key = character.front();
    Matrix matrix = matrixForCharacter(key);
    setFontPixel(key, row, column, !matrix[row][column]);
}

void LcdBackend::setCustomPixel(int index, int row, int column, bool active)
{
    if (index < 0 || index >= m_customChars.size() || row < 0 || row >= CharHeight || column < 0 || column >= CharWidth)
        return;
    const int value = active ? 1 : 0;
    if (m_customChars[index][row][column] == value)
        return;
    m_customChars[index][row][column] = value;
    emit customCharsChanged();
    emit displayChanged();
}

void LcdBackend::setFontPixel(const QString &character, int row, int column, bool active)
{
    if (character.isEmpty() || row < 0 || row >= CharHeight || column < 0 || column >= CharWidth)
        return;
    const QChar key = character.front();
    Matrix matrix = matrixForCharacter(key);
    const int value = active ? 1 : 0;
    if (matrix[row][column] == value)
        return;
    matrix[row][column] = value;
    m_font.insert(key, matrix);
    setStatus(QString("Символ %1 изменен").arg(key == ' ' ? QString("пробел") : QString(key)));
    emit fontChanged();
    emit displayChanged();
}

void LcdBackend::insertCustomChar(int index, int row, int column)
{
    if (index < 0 || index >= CustomCharCount || row < 0 || row >= Rows || column < 0 || column >= Columns)
        return;
    QString *line = row == 0 ? &m_line1 : &m_line2;
    while (line->size() < Columns)
        line->append(' ');
    (*line)[column] = QChar(0xE000 + index);
    emit displayChanged();
}

void LcdBackend::clearCustomChar(int index)
{
    if (index < 0 || index >= m_customChars.size())
        return;
    m_customChars[index] = blankMatrix();
    emit customCharsChanged();
    emit displayChanged();
}

void LcdBackend::saveProject(const QUrl &url)
{
    writeJsonFile(url, QJsonDocument(projectToObject()), "Проект сохранен");
}

void LcdBackend::loadProject(const QUrl &url)
{
    bool ok = false;
    const QJsonDocument document = readJsonFile(url, &ok);
    if (!ok)
        return;
    if (!document.isObject() || !loadProjectFromObject(document.object())) {
        setStatus("Не удалось прочитать проект");
        return;
    }
    setStatus("Проект загружен");
    emit customCharsChanged();
    emit fontChanged();
    emit displayChanged();
}

void LcdBackend::saveFont(const QUrl &url)
{
    writeTextFile(url, fontToBdf(), "Шрифт сохранен в BDF");
}

void LcdBackend::loadFont(const QUrl &url)
{
    bool ok = false;
    const QString content = readTextFile(url, &ok);
    if (!ok) {
        setStatus("Не удалось открыть шрифт");
        return;
    }

    if (content.trimmed().startsWith('{')) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(content.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            setStatus("Не удалось прочитать JSON-шрифт");
            return;
        }
        loadFontFromJson(document.object());
    } else if (!loadFontFromBdf(content)) {
        setStatus("Не удалось прочитать BDF-шрифт");
        return;
    }
    setStatus("Шрифт загружен");
    emit fontChanged();
    emit displayChanged();
}

void LcdBackend::exportPng(const QUrl &url)
{
    const int scale = qBound(1, m_exportScale, 8);
    const int dot = 4 * scale;
    const int gap = scale;
    const int charGap = 3 * scale;
    const int lineGap = 6 * scale;
    const int padding = 10 * scale;
    const int contentWidth = Columns * (CharWidth * dot + (CharWidth - 1) * gap) + (Columns - 1) * charGap;
    const int contentHeight = Rows * (CharHeight * dot + (CharHeight - 1) * gap) + (Rows - 1) * lineGap;
    const int framePadding = m_deviceFrame ? 12 * scale : 0;
    QImage image(contentWidth + (padding + framePadding) * 2,
                 contentHeight + (padding + framePadding) * 2,
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    render(painter, scale, m_deviceFrame);
    painter.end();

    const QString path = pathFromUrl(url);
    if (path.isEmpty()) {
        setStatus("Выберите файл для PNG");
        return;
    }
    if (!image.save(path, "PNG")) {
        setStatus("Не удалось сохранить PNG");
        return;
    }
    setStatus("PNG сохранен");
}

QString LcdBackend::projectJson() const
{
    return QString::fromUtf8(QJsonDocument(projectToObject()).toJson(QJsonDocument::Indented));
}

QString LcdBackend::normalizedLine(const QString &value) const
{
    QString result = value;
    result.replace('\n', ' ');
    result.replace('\r', ' ');
    if (result.size() > Columns)
        result.truncate(Columns);
    return result;
}

LcdBackend::Matrix LcdBackend::blankMatrix() const
{
    Matrix matrix;
    for (int row = 0; row < CharHeight; ++row)
        matrix.append(QVector<int>(CharWidth, 0));
    return matrix;
}

QVariantList LcdBackend::matrixToVariant(const Matrix &matrix) const
{
    QVariantList pixels;
    for (const auto &row : matrix) {
        for (int pixel : row)
            pixels.append(pixel);
    }
    return pixels;
}

QJsonArray LcdBackend::matrixToJson(const Matrix &matrix) const
{
    QJsonArray rows;
    for (const auto &row : matrix) {
        QJsonArray columns;
        for (int pixel : row)
            columns.append(pixel);
        rows.append(columns);
    }
    return rows;
}

LcdBackend::Matrix LcdBackend::matrixFromJson(const QJsonArray &array) const
{
    Matrix matrix = blankMatrix();
    if (array.size() == CharHeight && array.at(0).isArray()) {
        for (int row = 0; row < qMin(CharHeight, array.size()); ++row) {
            const QJsonArray columns = array.at(row).toArray();
            for (int column = 0; column < qMin(CharWidth, columns.size()); ++column)
                matrix[row][column] = columns.at(column).toInt() ? 1 : 0;
        }
    } else {
        for (int index = 0; index < qMin(CharWidth * CharHeight, array.size()); ++index)
            matrix[index / CharWidth][index % CharWidth] = array.at(index).toInt() ? 1 : 0;
    }
    return matrix;
}

QString LcdBackend::fontToBdf() const
{
    QString content;
    QTextStream stream(&content);
    stream << "STARTFONT 2.1\n";
    stream << "FONT -lcd1602-generator-fixed-medium-r-normal--8-80-75-75-c-50-iso10646-1\n";
    stream << "SIZE 8 75 75\n";
    stream << "FONTBOUNDINGBOX 5 8 0 0\n";
    stream << "STARTPROPERTIES 2\n";
    stream << "FONT_ASCENT 8\n";
    stream << "FONT_DESCENT 0\n";
    stream << "ENDPROPERTIES\n";
    stream << "CHARS " << m_font.size() << "\n";

    QList<QChar> keys = m_font.keys();
    std::sort(keys.begin(), keys.end(), [](QChar left, QChar right) {
        return left.unicode() < right.unicode();
    });

    for (QChar key : keys) {
        const Matrix matrix = m_font.value(key, blankMatrix());
        const ushort code = key.unicode();
        const QString name = key == ' ' ? "space" : QString("uni%1").arg(code, 4, 16, QLatin1Char('0'));
        stream << "STARTCHAR " << name << "\n";
        stream << "ENCODING " << code << "\n";
        stream << "SWIDTH 500 0\n";
        stream << "DWIDTH 6 0\n";
        stream << "BBX 5 8 0 0\n";
        stream << "BITMAP\n";
        for (int row = 0; row < CharHeight; ++row) {
            int byte = 0;
            for (int column = 0; column < CharWidth; ++column) {
                if (matrix[row][column])
                    byte |= 1 << (7 - column);
            }
            stream << QString("%1").arg(byte, 2, 16, QLatin1Char('0')) << "\n";
        }
        stream << "ENDCHAR\n";
    }
    stream << "ENDFONT\n";
    return content;
}

bool LcdBackend::loadFontFromBdf(const QString &content)
{
    QHash<QChar, Matrix> loaded;
    const QStringList lines = content.split(QRegularExpression("\\r?\\n"));
    int encoding = -1;
    int width = CharWidth;
    int height = CharHeight;
    QStringList bitmapRows;
    bool inBitmap = false;

    auto commitGlyph = [&]() {
        if (encoding < 0 || bitmapRows.isEmpty())
            return;
        Matrix matrix = blankMatrix();
        const int rowsToRead = qMin(CharHeight, qMin(height, bitmapRows.size()));
        for (int row = 0; row < rowsToRead; ++row) {
            bool ok = false;
            const int byte = bitmapRows.at(row).toInt(&ok, 16);
            if (!ok)
                continue;
            for (int column = 0; column < qMin(CharWidth, width); ++column)
                matrix[row][column] = (byte & (1 << (7 - column))) ? 1 : 0;
        }
        loaded.insert(QChar(encoding), matrix);
    };

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith("STARTCHAR")) {
            encoding = -1;
            width = CharWidth;
            height = CharHeight;
            bitmapRows.clear();
            inBitmap = false;
        } else if (trimmed.startsWith("ENCODING ")) {
            encoding = trimmed.mid(QString("ENCODING ").size()).section(' ', 0, 0).toInt();
        } else if (trimmed.startsWith("BBX ")) {
            const QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                width = parts.at(1).toInt();
                height = parts.at(2).toInt();
            }
        } else if (trimmed == "BITMAP") {
            inBitmap = true;
        } else if (trimmed == "ENDCHAR") {
            commitGlyph();
            bitmapRows.clear();
            inBitmap = false;
        } else if (inBitmap) {
            bitmapRows.append(trimmed);
        }
    }

    if (loaded.isEmpty())
        return false;
    for (auto it = loaded.cbegin(); it != loaded.cend(); ++it)
        m_font.insert(it.key(), it.value());
    return true;
}

LcdBackend::Matrix LcdBackend::matrixForCell(int cellIndex) const
{
    if (cellIndex < 0 || cellIndex >= Columns * Rows)
        return blankMatrix();
    const QString &line = cellIndex < Columns ? m_line1 : m_line2;
    const int column = cellIndex % Columns;
    if (column >= line.size())
        return blankMatrix();
    return matrixForCharacter(line.at(column));
}

LcdBackend::Matrix LcdBackend::matrixForCharacter(QChar character) const
{
    const ushort code = character.unicode();
    if (code >= 0xE000 && code < 0xE000 + CustomCharCount)
        return m_customChars[code - 0xE000];
    const QChar key = character;
    return m_font.value(key, blankMatrix());
}

QJsonObject LcdBackend::fontToJson() const
{
    QJsonObject object;
    for (auto it = m_font.cbegin(); it != m_font.cend(); ++it)
        object.insert(QString(it.key()), matrixToJson(it.value()));
    return object;
}

void LcdBackend::loadFontFromJson(const QJsonObject &object)
{
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.key().isEmpty() || !it.value().isArray())
            continue;
        m_font.insert(it.key().front(), matrixFromJson(it.value().toArray()));
    }
    emit fontChanged();
}

QJsonObject LcdBackend::projectToObject() const
{
    QJsonArray text;
    text.append(m_line1);
    text.append(m_line2);

    QJsonObject colors;
    colors.insert("bg", m_backgroundColor);
    colors.insert("pixel", m_pixelColor);
    colors.insert("backlight", m_backlightColor);

    QJsonArray customChars;
    for (const auto &matrix : m_customChars)
        customChars.append(matrixToJson(matrix));

    QJsonObject object;
    object.insert("format", "lcd1602-image-generator");
    object.insert("version", 1);
    object.insert("text", text);
    object.insert("colors", colors);
    object.insert("showGrid", m_showGrid);
    object.insert("deviceFrame", m_deviceFrame);
    object.insert("exportScale", m_exportScale);
    object.insert("customChars", customChars);
    object.insert("font", fontToJson());
    return object;
}

bool LcdBackend::loadProjectFromObject(const QJsonObject &object)
{
    const QJsonArray text = object.value("text").toArray();
    m_line1 = normalizedLine(text.size() > 0 ? text.at(0).toString() : QString());
    m_line2 = normalizedLine(text.size() > 1 ? text.at(1).toString() : QString());

    const QJsonObject colors = object.value("colors").toObject();
    m_backgroundColor = colors.value("bg").toString(m_backgroundColor);
    m_pixelColor = colors.value("pixel").toString(m_pixelColor);
    m_backlightColor = colors.value("backlight").toString(m_backlightColor);
    m_showGrid = object.value("showGrid").toBool(m_showGrid);
    m_deviceFrame = object.value("deviceFrame").toBool(m_deviceFrame);
    m_exportScale = qBound(1, object.value("exportScale").toInt(m_exportScale), 8);

    const QJsonArray customChars = object.value("customChars").toArray();
    for (int index = 0; index < qMin(CustomCharCount, customChars.size()); ++index)
        m_customChars[index] = matrixFromJson(customChars.at(index).toArray());

    const QJsonObject font = object.value("font").toObject();
    if (!font.isEmpty())
        loadFontFromJson(font);
    return true;
}

bool LcdBackend::writeJsonFile(const QUrl &url, const QJsonDocument &document, const QString &successMessage)
{
    const QString path = pathFromUrl(url);
    if (path.isEmpty()) {
        setStatus("Выберите файл");
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setStatus("Не удалось открыть файл");
        return false;
    }
    file.write(document.toJson(QJsonDocument::Indented));
    setStatus(successMessage);
    return true;
}

bool LcdBackend::writeTextFile(const QUrl &url, const QString &content, const QString &successMessage)
{
    const QString path = pathFromUrl(url);
    if (path.isEmpty()) {
        setStatus("Выберите файл");
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setStatus("Не удалось открыть файл");
        return false;
    }
    file.write(content.toUtf8());
    setStatus(successMessage);
    return true;
}

QJsonDocument LcdBackend::readJsonFile(const QUrl &url, bool *ok) const
{
    *ok = false;
    const QString path = pathFromUrl(url);
    if (path.isEmpty())
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    *ok = error.error == QJsonParseError::NoError;
    return document;
}

QString LcdBackend::readTextFile(const QUrl &url, bool *ok) const
{
    *ok = false;
    const QString path = pathFromUrl(url);
    if (path.isEmpty())
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    *ok = true;
    return QString::fromUtf8(file.readAll());
}

QString LcdBackend::pathFromUrl(const QUrl &url) const
{
    if (url.isLocalFile())
        return url.toLocalFile();
    return url.toString(QUrl::PreferLocalFile);
}

void LcdBackend::setStatus(const QString &message)
{
    if (m_status == message)
        return;
    m_status = message;
    emit statusChanged();
}

void LcdBackend::render(QPainter &painter, int scale, bool frame) const
{
    const int dot = 4 * scale;
    const int gap = scale;
    const int charGap = 3 * scale;
    const int lineGap = 6 * scale;
    const int padding = 10 * scale;
    const int contentWidth = Columns * (CharWidth * dot + (CharWidth - 1) * gap) + (Columns - 1) * charGap;
    const int contentHeight = Rows * (CharHeight * dot + (CharHeight - 1) * gap) + (Rows - 1) * lineGap;
    const int framePadding = frame ? 12 * scale : 0;
    QRect outer(0, 0, contentWidth + (padding + framePadding) * 2, contentHeight + (padding + framePadding) * 2);

    if (frame) {
        painter.fillRect(outer, QColor("#252b2f"));
        painter.fillRect(outer.adjusted(5 * scale, 5 * scale, -5 * scale, -5 * scale), QColor("#111416"));
    }

    QRect screen(framePadding, framePadding, contentWidth + padding * 2, contentHeight + padding * 2);
    painter.fillRect(screen, QColor(m_backlightColor));
    painter.fillRect(screen.adjusted(3 * scale, 3 * scale, -3 * scale, -3 * scale), QColor(m_backgroundColor));

    const QColor pixelColor(m_pixelColor);
    const QColor gridColor = QColor(pixelColor.red(), pixelColor.green(), pixelColor.blue(), 45);
    const int originX = framePadding + padding;
    const int originY = framePadding + padding;

    for (int cell = 0; cell < Columns * Rows; ++cell) {
        const Matrix matrix = matrixForCell(cell);
        const int lcdRow = cell / Columns;
        const int lcdColumn = cell % Columns;
        const int charX = originX + lcdColumn * (CharWidth * dot + (CharWidth - 1) * gap + charGap);
        const int charY = originY + lcdRow * (CharHeight * dot + (CharHeight - 1) * gap + lineGap);
        for (int row = 0; row < CharHeight; ++row) {
            for (int column = 0; column < CharWidth; ++column) {
                QRect pixel(charX + column * (dot + gap), charY + row * (dot + gap), dot, dot);
                if (m_showGrid)
                    painter.fillRect(pixel, gridColor);
                if (matrix[row][column])
                    painter.fillRect(pixel, pixelColor);
            }
        }
    }
}

void LcdBackend::initializeFont()
{
    m_font.insert(' ', blankMatrix());
    m_font.insert('A', makePattern({"01110", "10001", "10001", "11111", "10001", "10001", "10001", "00000"}));
    m_font.insert('B', makePattern({"11110", "10001", "10001", "11110", "10001", "10001", "11110", "00000"}));
    m_font.insert('C', makePattern({"01111", "10000", "10000", "10000", "10000", "10000", "01111", "00000"}));
    m_font.insert('D', makePattern({"11110", "10001", "10001", "10001", "10001", "10001", "11110", "00000"}));
    m_font.insert('E', makePattern({"11111", "10000", "10000", "11110", "10000", "10000", "11111", "00000"}));
    m_font.insert('F', makePattern({"11111", "10000", "10000", "11110", "10000", "10000", "10000", "00000"}));
    m_font.insert('G', makePattern({"01111", "10000", "10000", "10011", "10001", "10001", "01111", "00000"}));
    m_font.insert('H', makePattern({"10001", "10001", "10001", "11111", "10001", "10001", "10001", "00000"}));
    m_font.insert('I', makePattern({"11111", "00100", "00100", "00100", "00100", "00100", "11111", "00000"}));
    m_font.insert('J', makePattern({"00111", "00010", "00010", "00010", "00010", "10010", "01100", "00000"}));
    m_font.insert('K', makePattern({"10001", "10010", "10100", "11000", "10100", "10010", "10001", "00000"}));
    m_font.insert('L', makePattern({"10000", "10000", "10000", "10000", "10000", "10000", "11111", "00000"}));
    m_font.insert('M', makePattern({"10001", "11011", "10101", "10101", "10001", "10001", "10001", "00000"}));
    m_font.insert('N', makePattern({"10001", "11001", "10101", "10011", "10001", "10001", "10001", "00000"}));
    m_font.insert('O', makePattern({"01110", "10001", "10001", "10001", "10001", "10001", "01110", "00000"}));
    m_font.insert('P', makePattern({"11110", "10001", "10001", "11110", "10000", "10000", "10000", "00000"}));
    m_font.insert('Q', makePattern({"01110", "10001", "10001", "10001", "10101", "10010", "01101", "00000"}));
    m_font.insert('R', makePattern({"11110", "10001", "10001", "11110", "10100", "10010", "10001", "00000"}));
    m_font.insert('S', makePattern({"01111", "10000", "10000", "01110", "00001", "00001", "11110", "00000"}));
    m_font.insert('T', makePattern({"11111", "00100", "00100", "00100", "00100", "00100", "00100", "00000"}));
    m_font.insert('U', makePattern({"10001", "10001", "10001", "10001", "10001", "10001", "01110", "00000"}));
    m_font.insert('V', makePattern({"10001", "10001", "10001", "10001", "10001", "01010", "00100", "00000"}));
    m_font.insert('W', makePattern({"10001", "10001", "10001", "10101", "10101", "10101", "01010", "00000"}));
    m_font.insert('X', makePattern({"10001", "10001", "01010", "00100", "01010", "10001", "10001", "00000"}));
    m_font.insert('Y', makePattern({"10001", "10001", "01010", "00100", "00100", "00100", "00100", "00000"}));
    m_font.insert('Z', makePattern({"11111", "00001", "00010", "00100", "01000", "10000", "11111", "00000"}));
    m_font.insert('0', makePattern({"01110", "10001", "10011", "10101", "11001", "10001", "01110", "00000"}));
    m_font.insert('1', makePattern({"00100", "01100", "00100", "00100", "00100", "00100", "01110", "00000"}));
    m_font.insert('2', makePattern({"01110", "10001", "00001", "00010", "00100", "01000", "11111", "00000"}));
    m_font.insert('3', makePattern({"11110", "00001", "00001", "01110", "00001", "00001", "11110", "00000"}));
    m_font.insert('4', makePattern({"00010", "00110", "01010", "10010", "11111", "00010", "00010", "00000"}));
    m_font.insert('5', makePattern({"11111", "10000", "10000", "11110", "00001", "00001", "11110", "00000"}));
    m_font.insert('6', makePattern({"01110", "10000", "10000", "11110", "10001", "10001", "01110", "00000"}));
    m_font.insert('7', makePattern({"11111", "00001", "00010", "00100", "01000", "01000", "01000", "00000"}));
    m_font.insert('8', makePattern({"01110", "10001", "10001", "01110", "10001", "10001", "01110", "00000"}));
    m_font.insert('9', makePattern({"01110", "10001", "10001", "01111", "00001", "00001", "01110", "00000"}));
    m_font.insert('-', makePattern({"00000", "00000", "00000", "11111", "00000", "00000", "00000", "00000"}));
    m_font.insert('_', makePattern({"00000", "00000", "00000", "00000", "00000", "00000", "11111", "00000"}));
    m_font.insert('.', makePattern({"00000", "00000", "00000", "00000", "00000", "01100", "01100", "00000"}));
    m_font.insert(':', makePattern({"00000", "01100", "01100", "00000", "01100", "01100", "00000", "00000"}));
    m_font.insert('/', makePattern({"00001", "00010", "00010", "00100", "01000", "01000", "10000", "00000"}));
    m_font.insert('!', makePattern({"00100", "00100", "00100", "00100", "00100", "00000", "00100", "00000"}));
    m_font.insert('?', makePattern({"01110", "10001", "00001", "00010", "00100", "00000", "00100", "00000"}));
}
