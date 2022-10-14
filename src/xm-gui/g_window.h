#ifndef __GUI_EDITOR_WINDOW_H
#define __GUI_EDITOR_WINDOW_H

#include <string>
#include <list>

#include <oks/xm_dialog.h>

#include "g_object.h"
#include "g_context.h"

class OksDataEditorMainDialog;
class ConfdbGUIMainDialog;
class G_WindowInfo;
class G_Class;
class OksObject;
class OksClass;


class G_Window : public OksDialog
{
  friend class OksDataEditorMainDialog;
  friend class G_Object;

public:
  enum ObjectArrangeType {
    AsColumn,
    AsRow,
    ByClasses,
    WrapAny,
    WrapByClass
  };
  
  enum ChildArrangeType {
    ManyPerLine,
    OnePerLine
  };
  
  enum ShowState {
    ShowAll,
    ShowUsed,
    ShowNoneUsed,
    ShowNone
  };

  struct ShowInfo {
    const char *    name;
    const G_Class * g_class;
    ShowState	    show_state;
    bool	    show_extended;
    
    ShowInfo(const char *n, const G_Class *c, bool s_ext) :
      name		(n),
      g_class		(c),
      show_state	(ShowAll),
      show_extended	(s_ext) {
    }
  };


  ~G_Window		();

  const G_WindowInfo *	type() const {return p_win_info;}

  const std::string &	get_title() const {return title;}

  void			add_top_class(const G_Class *);
  void			remove_top_class(const G_Class *);
  bool			is_top_level(const G_Object *) const;

  void			add_use_datafile_classes(const G_Class *c);

  void			create_drawing_area();
  void			add_in_list_of_files();

  void			make_object_visible(const G_Object*) const;

  G_Object *		find_object(Dimension, Dimension) const;

  bool			operator==(const G_Window &odedd) const { return (d_area == odedd.d_area); }

  OksDataEditorMainDialog * get_parent() const {return parent;}
  G_Context *		get_context() {return &g_context;}

  void			draw(XExposeEvent *);
  void			input(XButtonEvent *);
  void			motion(Dimension, Dimension);

  bool			set_selected_file_active() const;

  G_Object *		create_top_level_object(OksObject *);
  void			insert_new_object(G_Object *);

  G_Object *		create_new_object(G_Class *, const char *, const char * = 0);
  G_Object *		create_new_object(OksObject *, bool);

  std::list<OksClass *> * get_new_object_types(G_Class *) const;
  std::list<OksClass *> * get_new_object_types(OksClass *) const;

  void			create_objects();
  void			create_child_objects(G_Object *, std::list<G_Object *>::iterator&, bool);

  void			delete_object(const OksObject *);
  void			remove_object_parent_link(const OksObject *, const OksObject *, const char *);
  void                  swap_rel_objects(OksObject *, const char *, const OksObject *, const OksObject *);
  void                  link_rel_objects(const char *, OksObject *, OksObject *);

  int		        show_references(const G_Object *);
  bool                  show_relationships(const G_Object *);

  void			change_used(std::list<OksObject *>&, bool);
  void			update_object_id(const OksObject *);

  void			sort_by_id();

  static void		start_auto_scrolling(G_Window *);
  static void		stop_auto_scrolling();
  
  void			init_new(G_Object *) {;}

    // this enum do not need to be in public section
  enum AddFile_Result {
    AddFile_NoAnswer,
    AddFile_Overwrite,
    AddFile_Add,
    AddFile_Cancel
  };

  G_Window		(const G_WindowInfo *, OksDataEditorMainDialog *);

  static void refresh(XtPointer p) {((G_Window *)p)->draw(0);}


protected:
  OksDataEditorMainDialog * parent;
  const G_WindowInfo *	p_win_info;
  std::string		title;

  Widget		d_area;
  G_Context		g_context;

  G_Object::G_Set objs_set;		// is used for fast search by oks object
  std::list<G_Object *> objects;	// is used for drawing (can't be replaced by above!)
  std::list<G_Object *> trash_objs;
  std::list<const G_Class *> top_classes;     // allow to create on top level
  std::list<const G_Class *> other_classes;   // allow to create childs
  std::list<const G_Class *> use_datafile_classes;
  std::list<Widget> file_sensible;


  void			add_shown_menu(const char *, const G_Class *, bool = false);

  void			append_file_sensible_widget(Widget w) { file_sensible.push_back(w); }

  enum {
    idDrawingForm = 101,
    idInFile,
/*    idArrangeAsRow,
    idArrangeAsColumn,
    idArrangeByClasses,
    idWrapAny,
    idWrapByClasses,
    idManyChildrenPerLine,
    idOneChildPerLine,*/
    idSortByID,
    idProperties,
    idPrint,
    idRefresh,
    idHelp,


      // do not add anything after these lines

    idMax		= 512,
    idShow		= 1000,
    idAddNew		= 9000
  };


private:
  G_Object *		dnd_dest_obj;
  ObjectArrangeType	obj_arrange_type;
  ChildArrangeType	child_arrange_type;
  std::list<ShowInfo>	show_infos;

  static XtIntervalId   auto_scrolling_timer;
  static G_Window *     auto_scrolling_window;

  static AddFile_Result p_add_file_result;

  void			add_new_data_file();
  bool			is_data_file_choosen() const;

  void			create_popup(XButtonPressedEvent *);

  std::list<const std::string *> * get_list_of_new_objects(const G_Object *) const;  
  bool			create_child_object(G_Object *, const std::string&, const std::string&);
  void			make_new_object_dlg(G_Object *, const std::string*, const std::string*);

  void			make_shown(unsigned long);

  void			create_new_datafile(const char *, const char *);
  
  void			remove_from_set(G_Object *);

  static void		drawCB(Widget, XtPointer , XtPointer);
  static void		resizeCB(Widget, XtPointer , XtPointer);
  static void		closeCB(Widget, XtPointer , XtPointer);
  static void		helpCB(Widget, XtPointer , XtPointer);
  static void		inputCB(Widget, XtPointer , XtPointer);
  static void		leaveAC(Widget, XtPointer , XEvent *, Boolean *);
  static void		motionAC(Widget, XtPointer , XEvent *, Boolean *);
  static void		changeDataFileCB(Widget, XtPointer, XtPointer);
  static void		newObjCB(Widget, XtPointer, XtPointer);
  static void		createNewDataFileCB(Widget, XtPointer, XtPointer);
  static void		dragCB(Widget, XtPointer, XtPointer);
  static void		dropCB(Widget, XtPointer, XtPointer);
  static void		transferCB(Widget, XtPointer, Atom *, Atom *, XtPointer, unsigned long *, int *);
  static void		dropDestroyCB(Widget, XtPointer, XtPointer);
  static void		popupAC(Widget, XtPointer , XEvent *, Boolean *);
  static void		popupCB(Widget, XtPointer , XtPointer);
  static void		autoScrollingCB(XtPointer, XtIntervalId*);
  static void		addFileCB(Widget, XtPointer , XtPointer);

  struct ObjAndWindow {
    G_Window * p_win;
    G_Object * p_obj;

    ObjAndWindow(G_Window * w, G_Object * o) : p_win(w), p_obj(o) {;}
  };

  static void           okShowRelsCB(Widget, XtPointer data, XtPointer);
  static void           cancelShowRelsCB(Widget, XtPointer data, XtPointer);
  void                  show_relationships_ob_object(G_Object *);

  static void		create_auto_scrolling_timer(G_Window *);
  static void		destroy_auto_scrolling_timer();
  static void		make_auto_scrolling(Widget, Dimension);
  
  static const char *	choose_file_name_str;
  static const char *	create_new_file_name_str;

  void			register_drop_site(Widget);
  void			unregister_drop_site(Widget);

  void			set_scrollbar_pos(Widget, Dimension) const;
};

#endif
