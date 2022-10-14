#ifndef __DATA_EDITOR_CLASS_DLG_H
#define __DATA_EDITOR_CLASS_DLG_H 

#include <list>

#include "data_editor_dialog.h"

class OksClass;
class OksObject;
class OksDataEditorSearchPanel;
class OksDataEditorMainDialog;


struct ObjectRow
{
  ObjectRow(const OksObject * obj, size_t size) :
      p_obj(obj)
  {
    p_values.reserve(size);
  }

  const OksObject * p_obj;
  std::vector<std::string> p_values;   // values of attributes and relationships
};

namespace ObjectTable
{
  void
  add_row(const OksObject *o, const OksClass * c, size_t sizeOfMatrix, std::list<ObjectRow *>& result);

  void
  show_rows(Widget matrix, size_t size, Widget pickup_bg_color, std::list<ObjectRow *>& result);

  Widget
  create_matrix(OksForm& f, const char * name, unsigned short id, const OksClass * c, OksDataEditorMainDialog * parent);

  void
  labelCB(Widget w, XtPointer oks_class, XtPointer cb);

  void
  objectCB(Widget w, XtPointer main_dlg, XtPointer cb);
}


class OksDataEditorClassDialog : public OksDataEditorDialog
{
  friend class OksDataEditorMainDialog;

  public:

    OksDataEditorClassDialog (OksClass*, OksDataEditorMainDialog*);

    ~OksDataEditorClassDialog ();
    OksDataEditorDialog::Type type() const {return OksDataEditorDialog::ClassDlg;}

    OksClass * get_class() const {return p_class;}


  private:

    OksClass *			p_class;
    OksDataEditorSearchPanel *  p_select_objects_panel;  // criteria to find objects by UID

    void refresh();
    static void add_row(Widget matrix, const OksObject *o); // used by notify-create()

    static void labelCB(Widget, XtPointer, XtPointer);
    static void objectCB(Widget, XtPointer, XtPointer);
    static void closeCB(Widget, XtPointer, XtPointer);
    static void actionCB(Widget, XtPointer, XtPointer);
    static void findCB(Widget, XtPointer, XtPointer);
    static void findObjectsCB(Widget, XtPointer, XtPointer);

    static void objectAC(Widget, XtPointer, XEvent *, Boolean *);

    enum {
      idNew = 110,
      idShow,
      idDelete,
      idFind,
      idQuery,
      idLoadQuery,
      idSelect
    };
};

#endif
