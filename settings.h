#ifndef SETTINGS_H
#define SETTINGS_H

#include <QByteArray>
#include <QColor>
#include <QFont>
#include <QList>
#include <QLocale>
#include <QMap>
#include <QObject>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QTranslator>

#include "scriptmodel.h"

struct HighlightRule
{
    HighlightRule(bool e, const QRegularExpression& r, const QColor& c)
        : enabled(e), regexp(r), color(c) {}
    HighlightRule() = default;

    bool enabled{false};
    QRegularExpression regexp;
    QColor color;

    inline bool operator==(const HighlightRule& hr) const
    {
        return (enabled == hr.enabled && regexp == hr.regexp && color == hr.color);
    }
};

enum TabPosition { North, South, West, East };

class Settings : public QObject
{
    Q_OBJECT
public:
    using TranslationMap = QMap<QLocale::Language, QMap<QLocale::Territory, QString>>;

    Settings();
    void load();

    inline const QString& getNotesPath() const               { return notes_path; }
    inline const QString& getLastNote() const                { return last_note; }
    inline bool getHideStart() const                         { return hide_start; }
    inline const QByteArray& getDialogGeometry() const       { return dialog_geometry; }
    inline const QByteArray& getDialogState() const          { return dialog_state; }
    inline TabPosition getTabPosition() const                { return tab_position; }
    inline bool getShowHidden() const                        { return show_hidden; }
    inline bool getShowExtensions() const                    { return show_extensions; }
    inline bool getHideFrame() const                         { return hide_frame; }
    inline bool getStayTop() const                           { return stay_top; }
    inline bool getSingleInstance() const                    { return single_instance; }
    inline bool getCopyStartRaise() const                    { return copy_start_raise; }
    inline const QFont& getNoteFont() const                  { return note_font; }
    inline const QFont& getHtmlFont() const                  { return html_font; }
    inline const ScriptModel& getScriptModel() const         { return script_model; }
    inline ScriptModel& getScriptModel()                     { return script_model; }
    inline bool getScriptShowOutput() const                  { return script_show_output; }
    inline bool getScriptCopyOutput() const                  { return script_copy_output; }
    inline const QList<int>& getToolbarItems() const         { return tb_items; }
    inline bool getNoteHighlight() const                     { return note_highlight; }
    inline const QList<HighlightRule>& getHighlightRules() const { return highlight_rules; }
    inline const TranslationMap& getTranslations() const     { return translations; }
    inline const QLocale& getLocaleCurrent() const           { return locale_current; }
    inline const QLocale& getLocale() const                  { return locale; }
    inline bool getLanguageCustom() const                    { return language_custom; }
    inline bool getNoteLinksOpen() const                     { return note_links_open; }
    inline bool getNotePastePlaintext() const                { return note_paste_plaintext; }
    inline bool getIconsUseSystemTheme() const               { return icons_use_system_theme; }
    inline int  getIconsSize() const                         { return icons_size; }

    void setNotesPath(const QString& path);
    void setLastNote(const QString& name);
    void setHideStart(bool hide);
    void setDialogGeometry(const QByteArray& g);
    void setDialogState(const QByteArray& g);
    void setTabPosition(TabPosition v);
    void setShowHidden(bool s);
    void setShowExtensions(bool s);
    void setHideFrame(bool v);
    void setStayTop(bool v);
    void setSingleInstance(bool v);
    void setCopyStartRaise(bool v);
    void setNoteFont(const QFont& font);
    void setHtmlFont(const QFont& font);
    void setScriptShowOutput(bool v);
    void setScriptCopyOutput(bool v);
    void setScripts();
    void setToolbarItems(const QList<int>& items);
    void setNoteHighlight(bool v);
    void setHighlightRules(const QList<HighlightRule>& rules);
    void setLocaleCurrent(const QLocale& loc);
    void setLocaleCustom(bool custom);
    void setNoteLinksOpen(bool v);
    void setNotePastePlaintext(bool v);
    void setIconsUseSystemTheme(bool v);
    void setIconsSize(int size);

    void loadLanguages();
    void updateLocale();

    inline void save() { config.sync(); }

private:
    QSettings config;
    QTranslator translator;
    QTranslator qtranslator;

    TranslationMap translations;
    TranslationMap qtTranslations;
    QLocale locale_current;
    QLocale locale_system;
    QLocale locale;
    bool language_custom{false};

    QString notes_path;
    QString last_note;
    bool hide_start{false};

    QByteArray dialog_geometry;
    QByteArray dialog_state;

    TabPosition tab_position{North};
    bool show_hidden{false};
    bool show_extensions{false};
    bool hide_frame{false};
    bool stay_top{false};

    bool single_instance{false};
    bool copy_start_raise{false};

    QFont note_font;
    bool note_links_open{false};
    bool note_paste_plaintext{false};
    QFont html_font;

    bool note_highlight{false};
    QList<HighlightRule> highlight_rules;

    ScriptModel script_model;
    bool script_show_output{false};
    bool script_copy_output{false};

    bool icons_use_system_theme{true};
    int icons_size{16};

    QList<int> tb_items;

signals:
    void NotesPathChanged();
    void ShowExtensionsChanged(bool show_extensions);
    void ShowHiddenChanged();
    void TabPositionChanged();
    void WindowStateChanged();
    void NoteFontChanged();
    void HtmlFontChanged();
    void NoteHighlightChanged();
    void NoteLinkOpenChanged();
    void NotePastePlaintextChanged();
    void tbHidingChanged();
    void ToolbarItemsChanged();
    void IconsSizeChanged();
};

extern Settings settings;

#endif // SETTINGS_H
