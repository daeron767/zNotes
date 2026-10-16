#include "aboutDialog.h"
#include "ui_aboutDialog.h"
#include "shared.h"

#include <QShortcut>

#ifndef VERSION
#define VERSION "unknown"
#endif

// Helper function to cleanly format credit entries with optional email addresses
static QString formatPerson(const QString& name, const QString& email, const QString& role)
{
    if (email.isEmpty()) {
        return QString("<p><b>%1</b><br/>%2</p>").arg(name, role);
    }
    return QString("<p><b>%1</b> - <a href='mailto:%2'>%2</a><br/>%3</p>").arg(name, email, role);
}

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::aboutDialog)
{
    ui->setupUi(this);

    // Adding program information
    ui->lbAbout->setText(QString("zNotes\n%1\n%2 %3\n%4\n%5\n\n%6")
        .arg(tr("Simple Qt-based notes application"))
        .arg(tr("version"), VERSION)
        .arg(tr("Copyright (c) 2021"))
        .arg(tr("by Peter Savichev (proton)"))
        .arg(tr("Distributed under the GPL license version 3 or later")));

    // Adding authors information
    QString strAuthors;
    strAuthors.append(formatPerson("Peter Savichev (proton)", "psavichev@gmail.com", tr("Application concept and main development")));
    strAuthors.append(QString("<p><b>%1</b></p>").arg(tr("Translations:")));
    strAuthors.append(formatPerson("Marcio Moraes (marciopanto)", "marciopanto@gmail.com", tr("Brazilian Portuguese translation")));
    strAuthors.append(formatPerson("Pavel Fric (fri)", "pavelfric@seznam.cz", tr("Czech translation")));
    strAuthors.append(formatPerson("Jan Rimmek", "jan@rimmek.de", tr("German translation")));
    strAuthors.append(formatPerson("Peter Savichev (proton)", "psavichev@gmail.com", tr("Russian translation")));
    strAuthors.append(formatPerson("Slavko Fedorik", "slavino@slavino.sk", tr("Slovak translation")));
    strAuthors.append(formatPerson("Alieola", "aliaeolova@gmail.com", tr("Spanish translation")));
    strAuthors.append(formatPerson("lukethenuke", "", tr("Swedish translation")));
    strAuthors.append(formatPerson("Vadim Nekhai (onix)", "onix@onix.name", tr("Ukrainian translation")));
    strAuthors.append(formatPerson("Janusz J. Maczka (zzzzzzzzz@github)", "", tr("Polish translation")));
    strAuthors.append(formatPerson("Heimen Stoffels", "vistausss@fastmail.com", tr("Dutch translation")));
    
    strAuthors.append(QString("<p><b>%1</b></p>").arg(tr("Icons:")));
    strAuthors.append(QString("<p>%1<br/><a href='%2'>%2</a></p>")
        .arg(tr("All icons are modified icons from Oxygen project"), "http://www.oxygen-icons.org"));
    
    ui->lbAuthors->setHtml(strAuthors);

    // Adding credits information
    QString strCredits;
    strCredits.append(QString("<p><b>%1</b></p>").arg(tr("Testing:")));
    strCredits.append(formatPerson("Nikolay Edigaryev", "edigaryev@gmail.com", tr("Tester")));
    strCredits.append(formatPerson("Nikita Lyalin (tinman321)", "tinman321@gmail.com", tr("Tester")));
    strCredits.append(formatPerson("Elbert Pol", "e.pol@chello.nl", tr("Tester")));
    strCredits.append(formatPerson("Andrew Bodrow (Drino)", "drinob@gmail.com", tr("Tester")));
    strCredits.append(formatPerson("Max Porozkov (MaxPv)", "maxpv@rusnix.ru", tr("Tester")));
    
    strCredits.append(QString("<p><b>%1</b></p>").arg(tr("Releasing:")));
    strCredits.append(formatPerson("Peter Savichev (proton) & Ben de Groot (yngwin)", "psavichev@gmail.com", "Gentoo"));
    strCredits.append(formatPerson("Nikita Lyalin (tinman321)", "tinman321@gmail.com", "ArchLinux"));
    strCredits.append(formatPerson("Eugene Pivnev (ti.eugene)", "ti.eugene@gmail.com", "Fedora, Mandriva, OpenSuse, Ubuntu"));
    strCredits.append(formatPerson("Veniamin Gvozdikov (zloidemon)", "g.veniamin@googlemail.com", "FreeBSD"));
    strCredits.append(formatPerson("Oktay Cetinkaya (xelat)", "xelat09@yahoo.de", "Windows"));
    strCredits.append(formatPerson("Elbert Pol", "e.pol@chello.nl", "OS/2"));
    
    ui->lbCredits->setHtml(strCredits);

    // Adding help information
    QString strHelp;
    strHelp.append(QString("<p><b>%1</b></p>").arg(tr("Shortcuts:")));
    strHelp.append(QString("<b>%1</b> - %2<br/>").arg("Alt+Fn", tr("Go to note n")));

    // Modern C++ range-based loop
    for (QShortcut* shortcut : Shared::shortcuts()) {
        if (shortcut && !shortcut->whatsThis().isEmpty()) {
            strHelp.append(QString("<b>%1</b> - %2<br/>")
                .arg(shortcut->key().toString(), shortcut->whatsThis()));
        }
    }
    
    ui->lbHelp->setHtml(strHelp);
}

aboutDialog::~aboutDialog()
{
    delete ui;
}
