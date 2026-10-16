#include "picturenote.h"

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>

PictureNote::PictureNote(const QFileInfo &fileinfo, Note::Type type_new, QObject *parent)
    : Note(fileinfo, type_new, parent)
{
    scroll_area = new QScrollArea();
    scroll_area->setAlignment(Qt::AlignCenter);

    label = new QLabel(scroll_area);
    label->setScaledContents(false);

    scroll_area->setWidget(label);

    load(); // loading note's content
}

PictureNote::~PictureNote()
{
    // scroll_area takes ownership of label via setWidget().
    // Deleting scroll_area will automatically clean up label.
    if (scroll_area)
    {
        scroll_area->deleteLater();
        scroll_area = nullptr;
    }
}

// Reading file
void PictureNote::load()
{
    if (file.isOpen())
    {
        file.close();
    }

    QImage image(file.fileName());
    if (!image.isNull())
    {
        label->setPixmap(QPixmap::fromImage(image));
        label->adjustSize();
    }
    else
    {
        label->setText(tr("Unable to load image: %1").arg(file.fileName()));
    }
}

// Saving note
void PictureNote::save(bool forced)
{
    if (!(content_changed || forced))
        return;

    if (file.isOpen())
    {
        file.close();
    }

    // Picture notes are currently read-only
    content_changed = false;
}

// Returning widget (can be placed in QTabWidget)
QWidget* PictureNote::widget()
{
    return scroll_area;
}

// Copying note's content to clipboard
void PictureNote::copy() const
{
    if (!label)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPixmap pix = label->pixmap();
    if (!pix.isNull())
    {
        QApplication::clipboard()->setPixmap(pix);
    }
#else
    const QPixmap *pix = label->pixmap();
    if (pix && !pix->isNull())
    {
        QApplication::clipboard()->setPixmap(*pix);
    }
#endif
}

bool PictureNote::isDocumentSupported() const
{
    return false;
}

QTextDocument* PictureNote::document() const
{
    return nullptr;
}
