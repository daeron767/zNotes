#ifndef NOTE_XML_H
#define NOTE_XML_H

#include "note.h"

#include <QDomDocument>
#include <memory>

class QTreeWidget;
class QDomElement;
class QTreeWidgetItem;

class XmlNote : public Note
{
    Q_OBJECT

public:
    XmlNote(const QFileInfo &fileinfo, Note::Type type_new, QObject *parent = nullptr);
    ~XmlNote() override;

    void load() override;                         // Reading note's content
    void save(bool forced = false) override;     // Saving note's content
    void copy() const override;                   // Copying note's content to clipboard
    QWidget* widget() override;

    bool isDocumentSupported() const override { return false; }
    QTextDocument* document() const override { return nullptr; }

private:
    QTreeWidget *tree_widget = nullptr;
    QDomDocument m_domDocument;

    void appendTask(const QDomElement &element, QTreeWidgetItem *parent = nullptr);
};

#endif // NOTE_XML_H
