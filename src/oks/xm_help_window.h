#ifndef __OKS_HELP_WINDOW_H
#define __OKS_HELP_WINDOW_H

#include <string>
#include <vector>
#include <set>

#include <oks/defs.h>

#include <oks/xm_dialog.h>

class OksXmHelpWindow;

class HistoryPair
{
  public:

    HistoryPair	  (const char *s1, const char *s2) : p_url(s1), p_title(s2) {};

    const std::string& url() const {return p_url;}
    const std::string& title() const {return p_title;}


  private:

    std::string p_url;
    std::string p_title;
};


class OksXmHelpWindow : public OksDialog
{
  public:

    enum {
      idHelpWnd = 100,
      idHelpWndInfoBar,
      idHelpWndCurrentURL,

      idBack,
      idForward,
      idGo,
      idReloadDocument,
      idReload_images,
      idRefresh,
      idInfo
    };

    OksXmHelpWindow			(const char*, OksTopDialog *);
    ~OksXmHelpWindow			();

    void				displayURL(const char *);

    void				showHelpWindow();
    void				redisplay() const;

    const std::string&			cache_dir() const {return p_cache_dir;}

    void				addToHistory(const char *, const char *);
    void				makeHistoryPopup(Widget, XButtonPressedEvent*) const;

    unsigned short			timeout() const {return p_timeout;}
    unsigned short			number_of_attempts() const {return p_number_of_attempts;}


  private:

    std::string p_cache_dir;
    bool p_help_window_is_shown;
    Widget html_w;

    std::vector<HistoryPair *> history;
    std::set<std::string, std::less< std::string > > visited;
    size_t current_pos;

    unsigned short p_timeout;
    unsigned short p_number_of_attempts;

    OksTopDialog * p_top_win;

    void				add_nav_button(short, const char *);
    void				refresh_nav_buttons() const;
    void				display_doc_info() const;

    void				reload_document() const;
    void				reload_images() const;

    void				add_visisted(const std::string&);
    bool				was_visited(const std::string&) const;

    void				back();
    void				forward();
    void				set_position(size_t);

    void				add_info_text(short, const char *);

    static void				add_info_item(OksDialog *, const char *, const char *, short);
    static bool				test_anchor(Widget, String);
    static std::string			set_absolute_url(const char *, bool = true);
    static void * /* XmImageInfo* */	load_image(Widget, const char *);
    static void				load_url(Widget, const char *);

    static void				unmapAC(Widget, XtPointer, XEvent *, Boolean *);
    static void				trackCB(Widget, XtPointer, XtPointer);
    static void				anchorCB(Widget, XtPointer, XtPointer);
    static void				historyCB(Widget, XtPointer, XtPointer);
    static void				navigateCB(Widget, XtPointer, XtPointer);
    static void				info_closeCB(Widget, XtPointer d, XtPointer);

};


#endif
