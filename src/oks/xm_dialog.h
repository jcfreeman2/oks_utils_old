#ifndef __OKS_XM_DIALOG
#define __OKS_XM_DIALOG

#include <list>

#include <Xm/Xm.h>


class OksForm;
class OksPanedWindow;
class OksDialog;
class OksTopDialog;


	//
	// This structure describes primitive window constols (label, text, text field,
	// separator, push button, toggle button, list, matrix), window manager widgets
	// (form, paned window, menu, option menu) and dialogs
	//

struct OksDlgEntity {
  enum Type {
    Unknown,
    TopLevelDialog,
    Dialog,
    Form,
    PanedWindow,
    PanedWindowWidget,
    Label,
    TextField,
    Text,
    Separator,
    PushButton,
    ToggleButton,
    List,
    Matrix,
    OptionMenu,
    ComboBox,
    Menu,
    ToolBar,
    StatusBar,
    ScrolledWindow
  };

  enum {
    idLabel       = -1,
    idSeparator   = -2,
    idPixmap      = -3,
    idCloseButton = 0x7FFF
  };

  static const char * noTitleAndScrollBars;

  short		id;
  void		*data;
  Type		type;

  union {
    Widget	     w;
    OksForm *	     f;
    OksPanedWindow * p;
  } d;
  
  bool	operator==(const OksDlgEntity & t) const {return (t.id == id);}
  Widget	get_parent_widget() const;
};


	//
	// Describes interface to XmForm widget
	// Contains a lot of usefull methods to create and handle controls and nested
	// window manager widget
	//

class OksForm
{
  public:

    OksForm		();
    OksForm		(Widget);
    virtual ~OksForm	();
  
    void		show() const;

    Widget		add_form(short id, const char *d = 0) {return add(OksDlgEntity::Form, id, (void *)d);}
    Widget		add_label(short id, const char *d = 0) {return add(OksDlgEntity::Label, id, (void *)d);}
    Widget		add_list(short id, const char *d = 0) {return add(OksDlgEntity::List, id, (void *)d);}
    Widget		add_matrix(short id, void *d = 0) {return add(OksDlgEntity::Matrix, id, d);}
    Widget		add_menubar() {return add(OksDlgEntity::Menu, 0);}
    Widget		add_option_menu(short id, std::list<const char *> *d) {return add(OksDlgEntity::OptionMenu, id, (void *)d);}
    Widget              add_combo_box(short id, std::list<const char *> *d) {return add(OksDlgEntity::ComboBox, id, (void *)d);}
    Widget		add_paned_window(short id, const char *d = 0) {return add(OksDlgEntity::PanedWindow, id, (void *)d);}
    Widget		add_push_button(short id, const char *d = 0) {return add(OksDlgEntity::PushButton, id, (void *)d);}
    Widget		add_separator() {return add(OksDlgEntity::Separator, OksDlgEntity::idSeparator);}
    Widget		add_simple_form(short id) {return add(OksDlgEntity::Form, id, (void *)OksDlgEntity::noTitleAndScrollBars);}
    Widget		add_text(short id, const char *d = 0) {return add(OksDlgEntity::Text, id, (void *)d);}
    Widget		add_text_field(short id, const char *d = 0) {return add(OksDlgEntity::TextField, id, (void *)d);}
    Widget		add_toggle_button(short id, const char *d = 0) {return add(OksDlgEntity::ToggleButton, id, (void *)d);}
    Widget		add_radio_button(short, const char * = 0);

    void		attach_previous(short, int = 0xFFFF, bool set_mx = true) const;
    void		attach_previous(Widget id, Widget left_w, Widget top_w, XtArgVal dy, XtArgVal attachment_t) const;
    void		attach_right(short) const;
    void		attach_bottom(short) const;

    Widget		get_form_widget() const {return form;}
    Widget		get_previous(short) const;

    Widget		get_widget(short) const;
    OksForm*		get_form(short) const;
    OksPanedWindow*	get_paned_window(short) const;
    OksDlgEntity::Type	get_type(short) const;

    void		hide_w(short id) const {map_widget(id, false);}
    void		show_w(short id) const {map_widget(id, true);}

    const std::list<OksDlgEntity*> & get_items() const {return entities;}

    Widget		set_value(short, const char *, int pos = 0, bool = true);
    char*		get_value(short) const;

    void		set_min_height() const;
    void		set_option_menu_cb(short, XtCallbackProc, XtPointer) const;

    void                change_option_menu(short id, std::list<const char *>& data);
    void                change_combo_box(short id, std::list<const char *>& data);

  protected:

    Widget		form;
    Widget		add(OksDlgEntity::Type, short, void * = 0);


  private:

    std::list<OksDlgEntity*> entities;

    void		map_widget(short, bool) const;
};


	//
	// Describes XmPanedWindow widget
	//

class OksPanedWindow
{
  public:

    OksPanedWindow	() {;}
    ~OksPanedWindow	();

    void		add_form(OksForm *, short);
    OksForm*		get_form(short) const;


  private:

    std::list<OksDlgEntity*> items;
};


	//
	// Describes an application dialog
	//

class OksDialog : public OksForm
{
  public:
    OksDialog	();
    OksDialog	(const char *, const OksTopDialog *);
    virtual ~OksDialog ();

    void	show(XtGrabKind type = XtGrabNone) const;

    void	setMinSize() const;
    void	setCascadePos() const;
    static void resetCascadePos(Widget w, Dimension new_x, Dimension new_y);
    void	setMaxSize(Dimension, Dimension) const;
    void	setTitle(const char *, const char * = 0, const char * = 0) const;
    void	setIcon(Pixmap) const;
    void	setIcon(OksDialog *) const;
    Widget	addCloseButton(const char * msg = "Close");

    void	createOksDlgHeader(Pixmap *, XtEventHandler, void *, short, const char *, short, const char *, short = 0, const char * = 0);
};
  

	//
	// Describes the top application dialog ("main window")
	//

class OksTopDialog : public OksDialog
{
  public:

    OksTopDialog	(const char *, int *, char **, const char *, const char * [][2] = 0);
    ~OksTopDialog	();

    Display *		display() const {return p_display;}
    Widget		app_shell() const {return p_app_shell;}
    XtAppContext	app_context() const {return p_app_context;}
    const char * 	app_class_name() const {return p_app_class_name;}


  protected:

    Display *		p_display;
    Widget		p_app_shell;
    XtAppContext	p_app_context;
    const char * 	p_app_class_name;
    
    char **             p_fallback_resources;
};


#endif
