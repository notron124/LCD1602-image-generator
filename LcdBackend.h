#pragma once

#include <QColor>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QPainter>
#include <QUrl>
#include <QVariantList>

class LcdBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString line1 READ line1 WRITE setLine1 NOTIFY displayChanged)
    Q_PROPERTY(QString line2 READ line2 WRITE setLine2 NOTIFY displayChanged)
    Q_PROPERTY(QString backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY displayChanged)
    Q_PROPERTY(QString pixelColor READ pixelColor WRITE setPixelColor NOTIFY displayChanged)
    Q_PROPERTY(QString backlightColor READ backlightColor WRITE setBacklightColor NOTIFY displayChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY displayChanged)
    Q_PROPERTY(bool deviceFrame READ deviceFrame WRITE setDeviceFrame NOTIFY displayChanged)
    Q_PROPERTY(int exportScale READ exportScale WRITE setExportScale NOTIFY displayChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString fontCharacters READ fontCharacters NOTIFY fontChanged)

public:
    using Matrix = QVector<QVector<int>>;

    explicit LcdBackend(QObject *parent = nullptr);

    QString line1() const;
    QString line2() const;
    QString backgroundColor() const;
    QString pixelColor() const;
    QString backlightColor() const;
    bool showGrid() const;
    bool deviceFrame() const;
    int exportScale() const;
    QString status() const;
    QString fontCharacters() const;

    void setLine1(const QString &value);
    void setLine2(const QString &value);
    void setBackgroundColor(const QString &value);
    void setPixelColor(const QString &value);
    void setBacklightColor(const QString &value);
    void setShowGrid(bool value);
    void setDeviceFrame(bool value);
    void setExportScale(int value);

    Q_INVOKABLE void applyTheme(const QString &themeId);
    Q_INVOKABLE QVariantList cellPattern(int cellIndex, int revision = 0) const;
    Q_INVOKABLE QVariantList customCharPattern(int index, int revision = 0) const;
    Q_INVOKABLE QVariantList fontCharPattern(const QString &character, int revision = 0) const;
    Q_INVOKABLE void ensureFontCharacters(const QString &characters);
    Q_INVOKABLE void toggleCustomPixel(int index, int row, int column);
    Q_INVOKABLE void toggleFontPixel(const QString &character, int row, int column);
    Q_INVOKABLE void setCustomPixel(int index, int row, int column, bool active);
    Q_INVOKABLE void setFontPixel(const QString &character, int row, int column, bool active);
    Q_INVOKABLE void insertCustomChar(int index, int row, int column);
    Q_INVOKABLE void clearCustomChar(int index);
    Q_INVOKABLE void saveProject(const QUrl &url);
    Q_INVOKABLE void loadProject(const QUrl &url);
    Q_INVOKABLE void saveFont(const QUrl &url);
    Q_INVOKABLE void loadFont(const QUrl &url);
    Q_INVOKABLE void exportPng(const QUrl &url);
    Q_INVOKABLE QString projectJson() const;

signals:
    void displayChanged();
    void customCharsChanged();
    void fontChanged();
    void statusChanged();

private:
    static constexpr int Columns = 16;
    static constexpr int Rows = 2;
    static constexpr int CharWidth = 5;
    static constexpr int CharHeight = 8;
    static constexpr int CustomCharCount = 8;

    QString normalizedLine(const QString &value) const;
    Matrix blankMatrix() const;
    QVariantList matrixToVariant(const Matrix &matrix) const;
    QJsonArray matrixToJson(const Matrix &matrix) const;
    Matrix matrixFromJson(const QJsonArray &array) const;
    QString fontToBdf() const;
    bool loadFontFromBdf(const QString &content);
    Matrix matrixForCell(int cellIndex) const;
    Matrix matrixForCharacter(QChar character) const;
    QJsonObject fontToJson() const;
    void loadFontFromJson(const QJsonObject &object);
    QJsonObject projectToObject() const;
    bool loadProjectFromObject(const QJsonObject &object);
    bool writeJsonFile(const QUrl &url, const QJsonDocument &document, const QString &successMessage);
    QJsonDocument readJsonFile(const QUrl &url, bool *ok) const;
    bool writeTextFile(const QUrl &url, const QString &content, const QString &successMessage);
    QString readTextFile(const QUrl &url, bool *ok) const;
    QString pathFromUrl(const QUrl &url) const;
    void setStatus(const QString &message);
    void initializeFont();
    void render(QPainter &painter, int scale, bool frame) const;

    QString m_line1;
    QString m_line2;
    QString m_backgroundColor = "#19381f";
    QString m_pixelColor = "#b6ff65";
    QString m_backlightColor = "#6ba842";
    bool m_showGrid = true;
    bool m_deviceFrame = true;
    int m_exportScale = 4;
    QString m_status = "Готово";
    QVector<Matrix> m_customChars;
    QHash<QChar, Matrix> m_font;
};
