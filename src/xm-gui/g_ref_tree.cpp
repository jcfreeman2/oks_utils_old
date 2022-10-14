#include "data_editor_main_dlg.h"
#include "g_ref_tree.h"

#include <oks/xm_utils.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Tree.h>


void
ConfdbGuiRefTree::closeCB(Widget, XtPointer client_data, XtPointer)
{
  delete (ConfdbGuiRefTree *)client_data;
}


void
ConfdbGuiRefTree::show_branch(Widget parent_w, G_Object::RefTree &ref_tree)
{
  std::string s(ref_tree.name);
  Widget w = 0;

  if(!s.empty()) {
    bool is_object = (s[0] == '\"' ? true : false);

    if(is_object) {
      s.erase(s.begin());
      s.erase(s.size() - 1);
      w = XmCreatePushButton(p_tree_w, (char *)s.c_str(), 0, 0);
    }
    else {
      w = XmCreateLabel(p_tree_w, (char *)"relationship", 0, 0);
    }

    XtVaSetValues(w, XmNfontList, p_gc->font_list, NULL);
    if(parent_w) XtVaSetValues(w, XmNsuperNode, parent_w, NULL);
    XtManageChild(w);

    if(is_object == false) {
      XmString xm_s = OksXm::create_string(s.c_str());
      XtVaSetValues(w, XmNlabelString, xm_s, NULL);
      XmStringFree(xm_s);
    }
  }

  for(std::list<G_Object::RefTree *>::iterator i = ref_tree.values.begin(); i != ref_tree.values.end(); ++i) {
    show_branch(w, *(*i));
  }
}


ConfdbGuiRefTree::ConfdbGuiRefTree(const G_Object * obj, const OksDataEditorMainDialog * p, G_Object::RefTree &ref_tree, G_Context * gc) :
  OksDialog	("", p),
  p_gc		(gc)
{
  std::string obj_name(G_Object::to_string(obj->get_oks_object()));

  setTitle("Reference Tree", obj_name.c_str());
  setIcon((OksDialog *)p);


    // create form contaning tree widget

  {
    std::string s("Object ");
    s += obj_name;
    s += " is referenced ";
    s += obj->number_of_copies_to_string();
    s += " times by:\n(objects are shown as buttons, relationships are shown as labels)";
    add_form(idTreeForm, s.c_str());
  }

  OksForm * f = get_form(idTreeForm);

  p_tree_w = (Widget)XsCreateTree(f->get_form_widget(), (char *)"Tree", 0, 0);

  XtVaSetValues(
    p_tree_w,
    XmNtopAttachment, XmATTACH_FORM,
    XmNtopOffset, 5,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, 5,
    XmNleftAttachment, XmATTACH_FORM,
    XmNleftOffset, 0,
    XmNrightAttachment, XmATTACH_FORM,
    XmNrightOffset, 5,
    XmNhorizontalSpace, 14,
    XmNverticalSpace, 2,
    NULL
  );

  show_branch(0, ref_tree);

  XtManageChild(p_tree_w);

  add_separator();

  XtAddCallback (addCloseButton(), XmNactivateCallback, closeCB, (XtPointer)this);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_right(idTreeForm);
  attach_bottom(idTreeForm);

  show();

  setCascadePos();
}
