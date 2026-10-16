#ifndef PICTURENOTE_H
#define PICTURENOTE_H

#include "note.h"

class QLabel;
class QScrollArea;

class PictureNote : public Note
{
    Q_OBJECT

public:
    explicit PictureNote(const QFileInfo &fileinfo,
                         Note::Type type_new,
                         QObject *parent = nullptr);
    ~PictureNote() override;

    void load() override;                         // Reading note's content
    void save(bool forced = false) override;      // Saving note's content
    void copy() const override;                   // Copying note's content to clipboard
    QWidget* widget() override;
    [[nodiscard]] bool isDocumentSupported() const override;
    [[nodiscard]] QTextDocument* document() const override;

private:
    QLabel *label{nullptr};
    QScrollArea *scroll_area{nullptr};
};

#endif // PICTURENOTE_H
