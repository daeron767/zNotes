#include "note_xml.h"

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QSaveFile>
#include <QTextStream>
#include <QTreeWidget>

XmlNote::XmlNote(const QFileInfo &fileinfo, Note::Type type_new, QObject *parent)
    : Note(fileinfo, type_new, parent)
{
    tree_widget = new QTreeWidget();
    tree_widget->setColumnCount(3);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    tree_widget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    tree_widget->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif

    load(); // Loading note's content
}

XmlNote::~XmlNote()
{
    // tree_widget is owned by the container UI parent (e.g. QTabWidget).
    // m_domDocument is automatically cleaned up on stack.
}

// Reading file
void XmlNote::load()
{
    if (file.isOpen())
        file.close();

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString errorMsg;
    int errorLine = 0;
    int errorColumn = 0;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_domDocument.setContent(&file, &errorMsg, &errorLine, &errorColumn))
#else
    if (!m_domDocument.setContent(&file, false, &errorMsg, &errorLine, &errorColumn))
#endif
    {
        file.close();
        return;
    }
    file.close();

    tree_widget->clear();
    QDomElement docElem = m_domDocument.documentElement();
    QDomNode n = docElem.firstChild();
    while (!n.isNull())
    {
        QDomElement e = n.toElement();
        if (!e.isNull() && e.tagName() == "task")
        {
            appendTask(e);
        }
        n = n.nextSibling();
    }
}

void XmlNote::appendTask(const QDomElement &element, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = nullptr;

    if (parent == nullptr)
    {
        item = new QTreeWidgetItem(tree_widget);
    }
    else
    {
        item = new QTreeWidgetItem(parent);
    }

    QDomNode n = element.firstChild();
    while (!n.isNull())
    {
        QDomElement e = n.toElement();
        if (!e.isNull())
        {
            if (e.tagName() == "title")
            {
                item->setText(0, e.text());
            }
            else if (e.tagName() == "task")
            {
                appendTask(e, item);
            }
        }
        n = n.nextSibling();
    }
}

// Saving note
void XmlNote::save(bool forced)
{
    if (!(content_changed || forced))
        return;

    QSaveFile saveFile(absolutePath());
    if (saveFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&saveFile);
        out << m_domDocument.toString();
        if (saveFile.commit())
        {
            content_changed = false;
        }
    }
}

// Returning widget (it can be placed in a tab widget)
QWidget* XmlNote::widget()
{
    return tree_widget;
}

// Copying note's content to clipboard
void XmlNote::copy() const
{
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard)
    {
        clipboard->setText(m_domDocument.toString());
    }
}
