#ifndef _CONFDB_GUI_G_OBJECT_
#define _CONFDB_GUI_G_OBJECT_

#include <string>
#include <list>
#include <set>
#include <vector>

#include <oks/defs.h>
#include <oks/exceptions.h>

#include "g_rectangle.h"
#include "g_tipstool.h"

class DragAndDropTransferResult;
class G_Class;
class G_Context;
class G_Window;
class OksString;
class OksClass;
class OksObject;
class OksRelationship;
class OksPopupMenu;

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>


class G_Kid {
  friend class G_Object;

  public:

    enum State {
      shown,
      hide
    };

    enum Cardinality {
      simple,
      multiple
    };
  
    enum Propagate {
      use_propagate,
      stop_propagate
    };

    virtual Cardinality	get_cardinality() const = 0;
    virtual ~G_Kid	() {;}

    bool		operator==(const G_Kid &c) const { return (rel_name == c.rel_name); }

    static Propagate	get_propagate_type(const OksClass *, const OksRelationship *);


  protected:

    G_Kid(const char * name, Propagate p, State s) :
      rel_name(name), propagate (p), state (s) {;}


  private:

    const char *	rel_name;	// children relationship name
    const Propagate	propagate;
    State		state;
};


class G_Object {
public:
  enum ComplexState {
    icon,	// show object as icon without attributes
    table	// show object as table with attributes
  };

  enum CompositeState {
    single,	// do not show composite children
    complete	// show composite children
  };
  
  enum DragAndDropState {
    none,
    src,
    dest_valid,
    dest_invalid
  };

  enum SystemMenuPopupID {
    idHide,
    idMinimize,
    idMaximize,
    idShowChildren,
    idHideChildren,
    idShowChildrenTopObject,
    idCopyReference,
    idSetLink,
    idReferencedBy,
    idContainedIn,
    idCopy,
    idRename,
    idModify,
    idDelete,
    idUnlinkWithParent,
    idAddNew  // note, it is the last !
  };

  enum DragAndDropPopupID {
    idRelationshipNone = 512,
    idRelationshipMove,
    idRelationshipSet
  };

  struct RefTree {
    std::string name;
    std::list<RefTree *> values;

    RefTree		(const std::string& s) : name(s) {};

    RefTree *		find(const std::string &) const;
    RefTree *		get(const std::string &);		// find existing or create new
  };

    // Declare set of poiters to G_Object sorted by OKS objects

  struct SortByOksObj {
    bool operator() (const G_Object * o1, const G_Object * o2) const {
      return o1->get_oks_object() < o2->get_oks_object();
    }
  };

  typedef std::multiset<G_Object *, SortByOksObj> G_Set;

  G_Object			(OksObject *, G_Context *, const G_Window *, const G_Class *,
                                 unsigned short, unsigned short,
                                 G_Kid::Propagate = G_Kid::use_propagate);
  G_Object			(const OksObject * o) :
                                 obj(const_cast<OksObject *>(o)),
				 propagate(G_Kid::stop_propagate) {;}
  ~G_Object			();

  static void			init(Widget w);

  bool				operator==(const G_Object &o) const { return (obj == o.obj); }
  bool				operator<(const G_Object &o) const { return (get_id() < o.get_id()); }

  G_Object *			make_copy(G_Kid::Propagate p) const {
    G_Object * nobj = new G_Object(get_oks_object(), get_gc(), p_window, g_class, 0, 0, p);
    nobj->number_of_copies = number_of_copies;
    return nobj;
  }

  const std::string&		get_id() const;
  const G_Class *		get_graphic_class() const {return g_class;}
  OksObject *			get_oks_object() const {return obj;}
  G_Context *			get_gc() const {return gc;}
  G_Kid::Propagate		get_propagate() const {return propagate;}
  
  bool                          is_one_child_per_row() const;

  std::string			get_attribute_value(const std::string&, int = 0) const;
  void				set_attribute_value(const std::string&, const char *);

  const std::string&		get_attribute_name(size_t i) const;
  const G_Rectangle&		get_attribute_rect(size_t i) const;

  std::string			get_icon_text() const;
  std::string			create_icon_text(const char *) const;

  DragAndDropState		get_dest_link_state() const;
  DragAndDropTransferResult *	get_user_dnd_action(OksObject *, XButtonEvent *) const;

  void				add_child(const std::string&, const std::string &);

  void		 		set_children(std::list<G_Object *> *, G_Set *);
  void		 		reset_children(G_Set *);
  void				unlink_children();
  bool				unlink_child(const OksObject *, const char *);

  void		                add_relationship_oks_object(OksObject *, const char *); // throw (oks::exception)
  static void	                add_relationship_oks_object(OksObject *, OksObject *, const char *); // throw (oks::exception)

  void		                remove_relationship_oks_object(OksObject *, const char *); // throw (oks::exception)
  static void	                remove_relationship_oks_object(OksObject *, OksObject *, const char *); // throw (oks::exception)

  const char *			get_reverse_relationship(const char *) const {return 0;}

  void				set_drag_and_drop_state(DragAndDropState s) {dnd_state = s;}
  DragAndDropState		get_drag_and_drop_state() const {return dnd_state;}

  static void			set_number_of_copies(G_Set *);
  unsigned int                  get_number_of_copies() const {return number_of_copies;} 
  std::string			number_of_copies_to_string() const;


	// input functions

  void				process_button_event(XButtonEvent *, G_Window *);
  int				process_motion_event(Dimension, Dimension);


	// GUI functions

  void				draw(bool = true);

  void				draw_children(bool);

  void				draw_icon(bool);
  void				draw_table(bool);

  bool				is_referenced() const {return (parent != 0);}
  G_Object *			get_parent() const {return parent;}

  void				set_invalid() {valid = false;}
  void				set_valid() {valid = true;}
  bool				is_valid() const {return valid;}
  
  bool				is_used() const {return used;}
  void				set_used(bool b) {used = b;}

  void				hide() {hiden = true;}
  void				show() {hiden = false;}
  bool				is_hiden() const {return hiden;}

  void				create_popup_menu(XButtonEvent *, G_Window *);

  static void			unpress_table_buttons();

  void				draw_table_system_btn();
  void				draw_table_system_btn_pressed();
  int				test_table_system_btn(Dimension, Dimension);

  void				draw_table_minimize_btn();
  void				draw_table_minimize_btn_pressed();
  int				test_table_minimize_btn(Dimension, Dimension);

  void				draw_children_btn();
  void				draw_children_btn_pressed();
  int				test_children_btn(Dimension, Dimension);

  int				test_table_class_name_btn(Dimension, Dimension);
  int				test_table_object_id_btn(Dimension, Dimension);

  void				set_icon_state() {complex_state = icon;}
  void				set_table_state() {complex_state = table;}

  void				show_children() {composite_state = complete;}
  void				hide_children() {composite_state = single;}

  ComplexState			get_complex_state() const {return complex_state;}
  CompositeState		get_composite_state() const {return composite_state;}

  bool				is_point_inside_object(unsigned short, unsigned short) const;
  bool				is_point_inside_composite_object(unsigned short, unsigned short) const;

  void				set_x(unsigned short i) {x = i;}
  unsigned short		get_x() const {return x;}

  void				set_y(unsigned short i) {y = i;}
  unsigned short		get_y() const {return y;}

  void				set_w(unsigned short i) {w = i;}
  unsigned short		get_w() const {return w;}

  void				set_h(unsigned short i) {h = i;}
  unsigned short		get_h() const {return h;}

  unsigned short		get_cw() const {return cw;}
  unsigned short		get_ch() const {return ch;}

  unsigned short		get_mw() const {return ( (cw != 0) ? cw : w );}
  unsigned short		get_mh() const {return ( (ch != 0) ? ch : h );}

  void				set_dx(unsigned short i) {dx = i;}
  unsigned short		get_dx() const {return dx;}

  void				set_dy(unsigned short i) {dy = i;}
  unsigned short		get_dy() const {return dy;}

  bool				is_shown() const {return shown;}
  void				set_non_shown() {shown = false;}

  bool				is_data_file_read_only() const;

  static void			clean_motion_timer(G_Context *);

  static void			remove_parent_link(G_Object *);

  static bool			is_url(const char *);
  static void			show_url(const OksString *);

  void                          swap_rel_objects(const char *, const OksObject *, const OksObject *);
  G_Object *			link_with_object(const char *, G_Object *);

  static void			update_object_id(const OksObject *);

  unsigned short		get_number_of_children() const {return number_of_children;}

  std::list<const std::string *> * get_list_of_new_objects() const;
  std::list<const std::string *> * get_list_of_new_objects(const std::list<const G_Class *> * top_classes,
                                                           const std::list<const G_Class *> * other_classes,
                                                           bool) const;

  void				init_new();

  static std::string		to_string(const OksObject *);

  static G_Object *		find_g_object(const OksObject *, G_Set *, bool = false);

  static OksObject *		get_clipboard_obj();
  static void			set_clipboard_obj(OksObject *o);
  
  void				get_references(const G_Object *, std::list<std::pair<const G_Object *, const char *> > *, RefTree &) const;

  void                          report_exception(const char * title, const oks::exception& ex) const;
  void                          report_error(const char * title, const char * message) const;



protected:
  OksObject *			obj;
  G_Context *			gc;
  const G_Window *		p_window;
  const G_Class *		g_class;
  const G_Kid::Propagate	propagate;
  bool				valid;
  bool				shown;
  bool				used;
  DragAndDropState		dnd_state;
  bool				hiden;

  ComplexState			complex_state;
  CompositeState		composite_state;

  std::list<G_Kid *>		children;

  unsigned short		x;
  unsigned short		y;
  unsigned short		w;
  unsigned short		h;
  unsigned short		cw;	// width with children
  unsigned short		ch;	// height with children
  unsigned short		iw;	// icon width without # of copies box
  unsigned short		dx;
  unsigned short		dy;

  G_Rectangle	 		num_of_copies_rect;  // is filled when icon is shown

  G_Rectangle	 		sys_btn;
  G_Rectangle	 		icon_btn;
  G_Rectangle	 		child_btn;
  G_Rectangle	 		class_btn;
  G_Rectangle	 		id_btn;
  std::vector<G_Rectangle *>	attributes;

  std::list<G_TipsTool *>	tips;

  unsigned short		number_of_children;
  unsigned int                  number_of_copies;

  G_Object *			parent;

  bool				has_attribute(const std::string&) const;
  size_t			give_number_of_attributes() const;

  static void			systemButtonPopupCB(Widget, XtPointer, XtPointer);
  static void			systemButtonPopupUnmapNotifyAC(Widget, XtPointer, XEvent *, Boolean *);
  static void			dropPopupUnmapNotifyAC(Widget, XtPointer, XEvent *event, Boolean *);
  static void			toolTipsCB(XtPointer, XtIntervalId*);

  void			        draw_relationship_pixmap(Dimension &, Dimension, const G_Kid *, Dimension &, G_Rectangle&, bool);

  Dimension			get_icon_width_dx(const G_Object *) const;
  void				draw_kid(G_Object *, Dimension&, Dimension, bool);
  
private:
  static G_TipsTool *		last_tip;
  static G_Object *		object_with_pressed_button;
  static G_Object *		object_with_pressed_icon_button;
  static G_Object *		object_with_pressed_child_button;
  
  Widget			get_drag_icon(Pixel, Pixel) const;
  void				create_drag_context(XButtonEvent *);
  
  void				show_attr_info(size_t, XButtonEvent *);
  void				show_contained_in() const;

  void				clean();
  
  static bool			is_verbose();
  
  static Boolean		objDragDropConvert(Widget, Atom *, Atom *, Atom *, XtPointer *, unsigned long *, int *);
  static void			objDragDropFinishCB(Widget, XtPointer, XtPointer);
  static void			objDropFinishCB(Widget, XtPointer, XtPointer);
  static void			objDragMotionCB(Widget, XtPointer, XtPointer);
  static void			objOperationChangedCB(Widget, XtPointer, XtPointer);
  static void			dragPopupCB(Widget, XtPointer, XtPointer);
  static void			editCB(Widget, XtPointer, XtPointer);
  static void			urlCB(Widget, XtPointer, XtPointer);
  static void			renameHelpCB(Widget, XtPointer, XtPointer);

  static void                   stop_auto_scrolling();
  void				draw_selection(Dimension, Dimension, Dimension, Dimension, bool) const;
};


class G_Child : public G_Kid {
  friend class G_Object;

public:
  virtual Cardinality	get_cardinality() const {return simple;}

private:
  G_Child(const char * name, Propagate p, State s = shown) : G_Kid(name, p, s), object (0) {;}
  virtual ~G_Child() {;}

  G_Object * object;
};


class G_Children : public G_Kid {
  friend class G_Object;

public:
  virtual Cardinality	get_cardinality() const {return multiple;}

private:
  G_Children(const char * name, Propagate p, State s = shown) : G_Kid(name, p, s) {;}
  virtual ~G_Children() {
    objects.clear();
  }

  std::list<G_Object *> objects;
};


#endif
