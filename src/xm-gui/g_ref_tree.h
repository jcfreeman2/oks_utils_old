#ifndef _CONFDB_GUI_G_REF_TREE_
#define _CONFDB_GUI_G_REF_TREE_

#include "g_object.h"

class OksDataEditorMainDialog;
class G_Context;

class ConfdbGuiRefTree : public OksDialog {

  public:

    ConfdbGuiRefTree(const G_Object *, const OksDataEditorMainDialog *, G_Object::RefTree &, G_Context *);


  private:
  
    Widget		p_tree_w;
    G_Context *		p_gc;
  
    void		show_branch(Widget, G_Object::RefTree &);

    static void		closeCB(Widget, XtPointer, XtPointer);

    enum {
      idTreeForm = 200
    };
};

#endif
