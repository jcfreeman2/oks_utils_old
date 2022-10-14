//
// This file contains static methods of G_Objects class
// which do external operation in scope of all editor
// windows and graphical objects
//

#include <iostream>

#include <oks/relationship.h>

#include "g_object.h"
#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"

extern OksDataEditorMainDialog * mainDlg;


void
G_Object::remove_parent_link(G_Object *g_obj)
{
  G_Object *g_parent_obj = g_obj->parent;


    //
    // continue if parent is defined and it has children
    //

  if(g_parent_obj && !g_parent_obj->children.empty()) {

      //
      // find name of relationship via which
      // the object is linked with parent
      //

    const char *rel_name = 0;
    std::list<G_Kid *>::iterator i = g_parent_obj->children.begin();

    for(; i != g_parent_obj->children.end() && rel_name == 0; ++i) {
      G_Kid *c = *i;

      if(c->get_cardinality() == G_Kid::simple) {
        if((static_cast<G_Child *>(c))->object == g_obj) {
          rel_name = c->rel_name;
	  break;
        }
      }
      else {
        G_Children *child2 = static_cast<G_Children *>(c);

        std::list<G_Object *>::iterator j = child2->objects.begin();

        for(;j != child2->objects.end();++j) {
          if((*j) == g_obj) {
            rel_name = c->rel_name;
	    break;
          }
        }
      }
    }

    if(rel_name) {
      try {
        g_parent_obj->remove_relationship_oks_object(g_obj->get_oks_object(), rel_name);
        OksDataEditorMainDialog::remove_object_parent_link(
          g_obj->get_oks_object(),
          g_parent_obj->get_oks_object(),
          rel_name
        );
      }
      catch(oks::exception& ex) {
        g_obj->report_exception("Remove Parent Link", ex);
      }
    }
    else {
      std::ostringstream text;
      text << "cannot find name of relationship between parent object \'" << g_parent_obj->get_id() << "\' and child object \'" << g_obj->get_id() << '\'';
      ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
    }
  }
}

void
G_Object::update_object_id(const OksObject * oks_obj)
{
  OksDataEditorMainDialog::update_object_id(oks_obj);
}

bool
G_Object::is_verbose()
{
  return OksDataEditorMainDialog::is_verbose();
}

std::list<const std::string *> *
G_Object::get_list_of_new_objects() const
{
  return OksDataEditorMainDialog::get_list_of_new_objects(this);
}

void
G_Object::add_child(const std::string& class_name, const std::string & rel_name)
{
  OksDataEditorMainDialog::create_child_object(this, class_name, rel_name);
}

void
G_Object::show_url(const OksString * url)
{
  mainDlg->show_help_window(url->c_str());
}

void
G_Object::renameHelpCB(Widget, XtPointer, XtPointer)
{
  mainDlg->show_help_window("rename-object.html");
}


 // defined by the data_editor_object_dlg.cpp
 
extern OksObject * clipboardObject;

OksObject *
G_Object::get_clipboard_obj()
{
  return clipboardObject;
}

void
G_Object::set_clipboard_obj(OksObject *o)
{
  clipboardObject = o;
}
