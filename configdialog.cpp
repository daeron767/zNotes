#include "configdialog.h"
#include "ui_configdialog.h"
#include "settings.h"
#include "toolbarmodel.h"
#include "highlightrulemodel.h"

#include <QAbstractButton>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QPainter>
#include <QStyle>

#define SINGLE_INSTANCE

configDialog::configDialog(QWidget *parent) :
    QDialog(parent), 
    m_ui(new Ui::configDialog),
    t_items(std::make_unique<ToolbarItems>())
{
    m_ui->setupUi(this);

#ifndef SINGLE_INSTANCE
    m_ui->cb_SingleInstance->setDisabled(true);
#endif

    m_items = new ItemModel(*t_items, this);
    mt_items = new ItemToolbarModel(*t_items, this);

    // List Actions View setup
    m_ui->listActions->setModel(m_items);
    m_ui->listActions->setDragEnabled(true);
    m_ui->listActions->setAcceptDrops(true);
    m_ui->listActions->setDropIndicatorShown(true);
    m_ui->listActions->setDragDropMode(QAbstractItemView::DragDrop);

    // List Toolbar Actions View setup
    m_ui->listToolbarActions->setModel(mt_items);
    m_ui->listToolbarActions->setDragEnabled(true);
    m_ui->listToolbarActions->setAcceptDrops(true);
    m_ui->listToolbarActions->setDropIndicatorShown(true);
    m_ui->listToolbarActions->setDragDropMode(QAbstractItemView::DragDrop);

    // Populate Settings UI
    m_ui->ed_NotesPath->setText(settings.getNotesPath());
    m_ui->cmb_TabPosition->setCurrentIndex(settings.getTabPosition());
    m_ui->cb_ShowHidden->setChecked(settings.getShowHidden());
    m_ui->cb_ShowExtensions->setChecked(settings.getShowExtensions());
    m_ui->cb_HideStart->setChecked(settings.getHideStart());
    m_ui->cb_FrameHide->setChecked(settings.getHideFrame());
    m_ui->cb_StayTop->setChecked(settings.getStayTop());

#ifdef SINGLE_INSTANCE
    m_ui->cb_SingleInstance->setChecked(settings.getSingleInstance());
    m_ui->cb_CopyStartRaise->setChecked(settings.getCopyStartRaise());
#endif

    m_ui->tabScripts->setModel(&settings.getScriptModel());
    m_ui->tabScripts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_ui->cb_ScriptShowOutput->setChecked(settings.getScriptShowOutput());
    m_ui->cb_ScriptCopyOutput->setChecked(settings.getScriptCopyOutput());

    m_ui->lb_FontExample->setFont(settings.getNoteFont());

    m_ui->cb_NoteLinksOpen->setChecked(settings.getNoteLinksOpen());
    m_ui->cb_NotePastePlaintext->setChecked(settings.getNotePastePlaintext());

    m_ui->cb_NoteHighlight->setChecked(settings.getNoteHighlight());
    m_ui->highlightRulesTableView->setEnabled(settings.getNoteHighlight());
    connect(m_ui->cb_NoteHighlight, &QCheckBox::toggled, 
            m_ui->highlightRulesTableView, &QTableView::setEnabled);

    highlight_rule_model = new HighlightRuleModel(this, settings.getHighlightRules());
    m_ui->highlightRulesTableView->setModel(highlight_rule_model);
    m_ui->highlightRulesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(m_ui->highlightRulesTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &configDialog::currentHighlightRuleModelRowChanged);

    // Translations setup
    const auto& translations = settings.getTranslations();
    for (auto it = translations.cbegin(); it != translations.cend(); ++it)
    {
        QLocale::Language language = static_cast<QLocale::Language>(it.key());
        const auto& country_map = it.value();
        for (auto c_it = country_map.cbegin(); c_it != country_map.cend(); ++c_it)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QLocale::Territory country = static_cast<QLocale::Territory>(c_it.key());
#else
            QLocale::Country country = static_cast<QLocale::Country>(c_it.key());
#endif
            QLocale locale(language, country);
            QString text = QString("%1 (%2)").
                arg(QLocale::languageToString(language),
                    QLocale::territoryToString(country));
            m_ui->cmb_Language->addItem(text, locale);
        }
    }

    int current_locale_index = m_ui->cmb_Language->findData(settings.getLocaleCurrent());
    if (current_locale_index != -1) {
        m_ui->cmb_Language->setCurrentIndex(current_locale_index);
    } else if (m_ui->cmb_Language->count() > 0) {
        m_ui->cmb_Language->setCurrentIndex(0);
    }

    m_ui->cb_LanguageCustom->setChecked(settings.getLanguageCustom());
    m_ui->cmb_Language->setEnabled(settings.getLanguageCustom());

    t_items->setVector(settings.getToolbarItems());

    m_ui->cb_IconsSize->setCurrentText(QString("%1").arg(settings.getIconsSize()));
    m_ui->cb_IconsUseSystemTheme->setChecked(settings.getIconsUseSystemTheme());

#if defined Q_WS_X11 || defined Q_OS_LINUX
    m_ui->cb_IconsUseSystemTheme->show();
#else
    m_ui->cb_IconsUseSystemTheme->hide();
#endif

    connect(m_ui->listActions->selectionModel(), &QItemSelectionModel::currentRowChanged, 
            this, &configDialog::currentListActionChanged);
    connect(m_ui->listToolbarActions->selectionModel(), &QItemSelectionModel::currentRowChanged, 
            this, &configDialog::currentToolbarActionChanged);

#if defined(Q_OS_OS2)
    m_ui->cb_HideStart->setDisabled(true);
#endif

    // === Highlight Rules Buttons - Icons ===
    QPixmap plusPix(24, 24);
    plusPix.fill(Qt::transparent);
    QPainter p(&plusPix);
    p.setRenderHint(QPainter::Antialiasing);
    QFont font = p.font();
    font.setPixelSize(18);
    font.setBold(true);
    p.setFont(font);
    p.setPen(palette().color(QPalette::WindowText));
    p.drawText(plusPix.rect(), Qt::AlignCenter, "+");

    m_ui->highlightRuleAddButton->setIcon(QIcon(plusPix));
    m_ui->highlightRuleAddButton->setIconSize(QSize(24, 24));

    m_ui->highlightRuleRemoveButton->setIcon(
        QIcon::fromTheme("list-remove", style()->standardIcon(QStyle::SP_TrashIcon)));
}

configDialog::~configDialog()
{
    delete m_ui;
}

void configDialog::saveSettings()
{
    settings.setHideStart(m_ui->cb_HideStart->isChecked());
    settings.setNotesPath(m_ui->ed_NotesPath->text());
    settings.setTabPosition(TabPosition(m_ui->cmb_TabPosition->currentIndex()));
    settings.setShowHidden(m_ui->cb_ShowHidden->isChecked());
    settings.setShowExtensions(m_ui->cb_ShowExtensions->isChecked());
    settings.setHideFrame(m_ui->cb_FrameHide->isChecked());
    settings.setStayTop(m_ui->cb_StayTop->isChecked());
#ifdef SINGLE_INSTANCE
    settings.setSingleInstance(m_ui->cb_SingleInstance->isChecked());
    settings.setCopyStartRaise(m_ui->cb_CopyStartRaise->isChecked());
#endif
    settings.setNoteFont(m_ui->lb_FontExample->font());
    settings.setNoteLinksOpen(m_ui->cb_NoteLinksOpen->isChecked());
    settings.setNotePastePlaintext(m_ui->cb_NotePastePlaintext->isChecked());
    settings.setNoteHighlight(m_ui->cb_NoteHighlight->isChecked());
    settings.setHighlightRules(highlight_rule_model->getRules());
    settings.setScriptShowOutput(m_ui->cb_ScriptShowOutput->isChecked());
    settings.setScriptCopyOutput(m_ui->cb_ScriptCopyOutput->isChecked());
    settings.setToolbarItems(t_items->getToolbarElems());
    settings.setLocaleCustom(m_ui->cb_LanguageCustom->isChecked());
    settings.setLocaleCurrent(m_ui->cmb_Language->itemData(m_ui->cmb_Language->currentIndex(), Qt::UserRole).toLocale());
    settings.setIconsSize(m_ui->cb_IconsSize->currentText().toInt());
    settings.setIconsUseSystemTheme(m_ui->cb_IconsUseSystemTheme->isChecked());
}

void configDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole role = m_ui->buttonBox->buttonRole(button);

    if (role == QDialogButtonBox::AcceptRole)
    {
        saveSettings();
        accept();
    }
    else if (role == QDialogButtonBox::RejectRole)
    {
        reject();
    }
    else if (role == QDialogButtonBox::ApplyRole)
    {
        saveSettings();
    }
}

void configDialog::on_btn_NotesPath_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this,
            tr("Select notes directory"), settings.getNotesPath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!path.isEmpty()) 
        m_ui->ed_NotesPath->setText(path);
}

void configDialog::on_btn_FontChange_clicked()
{
    QFont currentFont = m_ui->lb_FontExample->font();
    if (currentFont.pointSize() <= 0)
        currentFont.setPointSize(9);

    QFontDialog dlg(currentFont, this);
    dlg.setWindowTitle(tr("Select note font"));
    dlg.setOption(QFontDialog::ScalableFonts);
    if (dlg.exec() == QDialog::Accepted)
    {
        QFont selected = dlg.selectedFont();
        if (selected.pointSize() > 0)
        {
            m_ui->lb_FontExample->setFont(selected);
            settings.setNoteFont(selected);
        }
    }
}

void configDialog::on_btn_ScriptRemove_clicked()
{
    if (!m_ui->tabScripts->selectionModel()->hasSelection()) return;
    int row = m_ui->tabScripts->selectionModel()->currentIndex().row();
    settings.getScriptModel().removeRow(row);
}

void configDialog::on_btn_ScriptAdd_clicked()
{
    settings.getScriptModel().append();
}

void configDialog::on_butActionAdd_clicked()
{
    int id = m_ui->listActions->currentIndex().row();
    int row = m_ui->listToolbarActions->selectionModel()->hasSelection() ?
        m_ui->listToolbarActions->currentIndex().row() : -1;
    t_items->insert(id, row);
}

void configDialog::on_butActionRemove_clicked()
{
    m_ui->butActionRemove->setDisabled(true);
    QModelIndex index = m_ui->listToolbarActions->currentIndex();
    t_items->remove(index.row());
}

void configDialog::on_butActionTop_clicked()
{
    QModelIndex index(m_ui->listToolbarActions->currentIndex());
    QModelIndex new_index = mt_items->up(index);
    m_ui->listToolbarActions->setCurrentIndex(new_index);
}

void configDialog::on_butActionBottom_clicked()
{
    QModelIndex index(m_ui->listToolbarActions->currentIndex());
    QModelIndex new_index = mt_items->down(index);
    m_ui->listToolbarActions->setCurrentIndex(new_index);
}

void configDialog::currentToolbarActionChanged(QModelIndex index, QModelIndex)
{
    m_ui->butActionRemove->setEnabled(index.isValid());
    int row = index.row();
    m_ui->butActionTop->setEnabled(row > 0);
    m_ui->butActionBottom->setEnabled(row < (mt_items->rowCount() - 1));
}

void configDialog::currentListActionChanged(QModelIndex index, QModelIndex)
{
    m_ui->butActionAdd->setEnabled(index.isValid() && !t_items->isUsed(index.row()));
}

void configDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        m_ui->cmb_TabPosition->setCurrentIndex(settings.getTabPosition());
        break;
    default:
        break;
    }
}

void configDialog::on_highlightRuleAddButton_clicked()
{
    highlight_rule_model->appendRow();
}

void configDialog::on_highlightRuleRemoveButton_clicked()
{
    highlight_rule_model->removeRow(m_ui->highlightRulesTableView->currentIndex());
}

void configDialog::currentHighlightRuleModelRowChanged(QModelIndex index, QModelIndex)
{
    m_ui->highlightRuleRemoveButton->setEnabled(index.isValid());
    m_ui->highlightRuleUpButton->setEnabled(index.row() > 0);
    m_ui->highlightRuleDownButton->setEnabled(index.row() < (highlight_rule_model->rowCount() - 1));
}

void configDialog::on_highlightRuleUpButton_clicked()
{
    highlight_rule_model->up(m_ui->highlightRulesTableView->currentIndex());
    currentHighlightRuleModelRowChanged(m_ui->highlightRulesTableView->currentIndex());
}

void configDialog::on_highlightRuleDownButton_clicked()
{
    highlight_rule_model->down(m_ui->highlightRulesTableView->currentIndex());
    currentHighlightRuleModelRowChanged(m_ui->highlightRulesTableView->currentIndex());
}

void configDialog::changeTabToFirst()
{
    m_ui->tabWidget->setCurrentIndex(0);
}

void configDialog::changeTabToCommands()
{
    m_ui->tabWidget->setCurrentIndex(1);
}

