#ifndef NOTE_H
#define NOTE_H

#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QString>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QWidget>

class Note : public QObject
{
    Q_OBJECT

public:
    enum Type
    {
        type_text,
        type_html,
        type_picture,
#ifdef NOTE_TODO_FORMAT
        type_todo,
#endif
#ifdef NOTE_XML_FORMAT
        type_xml,
#endif
    };
    Q_ENUM(Type)

    explicit Note(const QFileInfo& fileinfo, Note::Type type_new, QObject *parent = nullptr);
    ~Note() override;

    [[nodiscard]] QString title() const { return _title; }
    [[nodiscard]] QString absolutePath() const { return file_info.absoluteFilePath(); }
    [[nodiscard]] QString fileName() const { return file_info.fileName(); }
    [[nodiscard]] Type type() const { return _type; }

    virtual QWidget* widget() = 0;
    void updateTitle(bool show_extensions);
    virtual void load() = 0;                  // Reading note's content
    virtual void save(bool forced = false) = 0;  // Saving note's content
    void rename(const QString &new_name);
    void move(const QString &new_dir);
    virtual void copy() const = 0;               // Copying note's content to clipboard

    // Unified Find and Highlight interfaces
    virtual bool find(const QString &text, 
                      QTextDocument::FindFlags flags = {}, 
                      bool from_start = false);

    virtual void highlightAll(const QString &text, 
                             QTextDocument::FindFlags flags = {});

    virtual void setSelFormat(const QTextCharFormat &format)
    {
        Q_UNUSED(format)
    }
    [[nodiscard]] virtual QTextCharFormat getSelFormat() const
    {
        return {};
    }

    virtual void retranslate(const QLocale &locale)
    {
        Q_UNUSED(locale)
    }
    [[nodiscard]] virtual bool isDocumentSupported() const = 0;
    [[nodiscard]] virtual QTextDocument* document() const = 0;

private:
    Type _type;
    QString _title;
    QFileInfo file_info;

protected:
    QFile file;
    bool content_changed{false};

protected slots:
    void contentChanged();
};

#endif // NOTE_H
