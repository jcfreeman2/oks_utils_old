#include <algorithm>

#include <ers/ers.h>

#include <oks/class.h>
#include <oks/object.h>

#include "g_class.h"
#include "g_object.h"

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"


extern OksDataEditorMainDialog * mainDlg;

void
OksDataEditorMainDialog::add_used(const OksData *d)
{
  if(d && d->type == OksData::object_type && d->data.OBJECT) {
    OksObject * o = d->data.OBJECT;
    if(used.find(o) == used.end()) {
      used.insert(o);

      const OksClass * c = o->GetClass();
      const size_t num_of_rels(c->number_of_all_relationships());

      if(num_of_rels) {
        std::map<OksClass*, size_t*, std::less<OksClass*> >::iterator pos = remove_from_used.find(const_cast<OksClass *>(c));
        size_t * ignore_offs = (pos != remove_from_used.end() ? (*pos).second : 0);

        size_t i = c->number_of_all_attributes();
        const size_t size_of_obj(i + num_of_rels);

	for(; i < size_of_obj; ++i) {
          if(ignore_offs && *ignore_offs == i) {
	    ignore_offs++;
	    continue;
	  }

	  OksDataInfo odi(i, (OksRelationship *)0);
	  OksData * d2(o->GetRelationshipValue(&odi));

          if(d2->type == OksData::list_type) {
            OksData::List * dlist = d2->data.LIST;

	    if(dlist && !dlist->empty()) {
	      for(OksData::List::iterator j = dlist->begin(); j != dlist->end(); ++j) {
	        add_used(*j);
	      }
	    }
	  }
	  else
	    add_used(d2);
        }
      }
    }
  }
}

void
OksDataEditorMainDialog::find_used()
{
  if(p_show_used) {
    used.clear();

    for(std::set<OksObject*>::const_iterator j = p_root_objects.begin(); j != p_root_objects.end(); ++j) {
      OksData d(*j);
      add_used(&d);
    }
  }
}


void
OksDataEditorMainDialog::reset_used()
{
  if(p_show_used == false) return;

  if(is_verbose()) {
    std::cout << "* reset used objects ...\n";
  }

  std::set<OksObject*, std::less<OksObject*> > old_used(used);

  find_used();

  std::set<OksObject*, std::less<OksObject*> >::iterator i;


    // find newly used

  std::list<OksObject *> add_to_used;
  std::list<OksObject *> delete_from_used;

  for(i = used.begin(); i != used.end(); ++i) {
    if(old_used.find(*i) == old_used.end()) {
      add_to_used.push_back(*i);
      if(is_verbose()) {
        std::cout << " + add to used oks object " << *i << std::endl;
      }
    }
  }

  for(i = old_used.begin(); i != old_used.end(); ++i) {
    if(used.find(*i) == used.end()) {
      delete_from_used.push_back(*i);
      if(is_verbose()) {
        std::cout << " - remove from used oks object " << (void *)(*i) << ' ';
	if(is_dangling(*i)) {
	  std::cout << "(dangling)\n";
	}
	else {
	  std::cout << *i << std::endl;
	}
      }
    }
  }


      // update graphic windows showing changed object

  for(std::list<G_Window *>::iterator j = p_windows.begin(); j != p_windows.end(); ++j) {
    (*j)->change_used(add_to_used, true);
    (*j)->change_used(delete_from_used, false);
  }

  if(is_verbose()) {
    std::cout << std::endl;
  }
}


    //
    // The method swaps two objects belonging to the same relationship:
    //  - shown objects on screen, and
    //  - in OKS database
    //

void
OksDataEditorMainDialog::swap_rel_objects(OksObject *parent, const char *rel_name, const OksObject *o1, const OksObject *o2)
{
    // swap shown graphical objects

  swap_objects_parent_link(parent, o1, o2, rel_name);


    // swap objects in oks

  try {
    OksData *d(parent->GetRelationshipValue(rel_name));

    for(OksData::List::iterator i = d->data.LIST->begin(); i != d->data.LIST->end(); ++i) {
      if((*i)->data.OBJECT == o1)
        (*i)->data.OBJECT = const_cast<OksObject *>(o2);
      else if((*i)->data.OBJECT == o2)
        (*i)->data.OBJECT = const_cast<OksObject *>(o1);
    }

    parent->SetRelationshipValue(rel_name, d);
  }
  catch(oks::exception& ex) {
    OksDataEditorMainDialog::report_exception("Swap relationship Objects", ex);
  }
}


void
OksDataEditorMainDialog::link_rel_objects(const char *rel_name, G_Object *parent_obj, OksObject *child_obj)
{
    // remember old object

  OksObject *old_obj = 0;
  
  try {
    {
      OksData *d(parent_obj->get_oks_object()->GetRelationshipValue(rel_name));
      if(d->type == OksData::object_type) old_obj = d->data.OBJECT;
    }

    parent_obj->add_relationship_oks_object(child_obj, rel_name);

      // unlink replaced object if it was
    if(old_obj) {
      ERS_DEBUG( 2, "OksDataEditorMainDialog::link_rel_objects(\'" << rel_name << "\', ...): remove link with object " << old_obj);
      remove_object_parent_link(old_obj, parent_obj->get_oks_object(), rel_name);
    }

      // update opened graphical windows to reflect link between objects
    add_object_parent_link(child_obj, parent_obj->get_oks_object(), rel_name);

      // check if we need to make some objects 'used'
    mainDlg->reset_used();
  }
  catch(oks::exception& ex) {
    parent_obj->report_exception("Link Objects", ex);
  }
}


void
OksDataEditorMainDialog::swap_objects_parent_link(OksObject * parent, const OksObject * o1, const OksObject * o2, const char * rel_name)
{
  for(std::list<G_Window *>::iterator j = mainDlg->p_windows.begin(); j != mainDlg->p_windows.end(); ++j)
    (*j)->swap_rel_objects(parent, rel_name, o1, o2);
}


void
OksDataEditorMainDialog::add_object_parent_link(OksObject * child_obj, OksObject * p_obj, const char * rel_name)
{
    // update opened graphical windows to reflect link between objects

  for(std::list<G_Window *>::iterator i = mainDlg->p_windows.begin(); i != mainDlg->p_windows.end(); ++i) {
    (*i)->link_rel_objects(rel_name, p_obj, child_obj);
  }
}


void
OksDataEditorMainDialog::remove_object_parent_link(OksObject * child_obj, OksObject * p_obj, const char * rel_name)
{
  for(std::list<G_Window *>::iterator i = mainDlg->p_windows.begin(); i != mainDlg->p_windows.end(); ++i) {
    (*i)->remove_object_parent_link(child_obj, p_obj, rel_name);
  }

  if(mainDlg->uses_object(child_obj))
    mainDlg->reset_used();
}


std::list<const std::string *> *
OksDataEditorMainDialog::get_list_of_new_objects(const G_Object * o)
{
  if(!mainDlg->p_windows.empty()) {
    for(std::list<G_Window *>::iterator i = mainDlg->p_windows.begin(); i != mainDlg->p_windows.end(); ++i) {
      std::list<const std::string *> * olist = (*i)->get_list_of_new_objects(o);
      if(olist) return olist;
    }
  }

  return 0;
}

void
OksDataEditorMainDialog::create_child_object(G_Object * o, const std::string& cn, const std::string& rn)
{
  for(std::list<G_Window *>::iterator i = mainDlg->p_windows.begin(); i != mainDlg->p_windows.end(); ++i) {
    if((*i)->create_child_object(o, cn, rn)) return;
  }
}


OksObject *
OksDataEditorMainDialog::find_oks_object(const char *class_name, const char *object_id) const
{
  const OksClass * oks_class = find_class(class_name);
  
  if(oks_class) {
    OksObject * oks_object = oks_class->get_object(object_id);

    if(oks_object)
      return oks_object;
    else {
      std::ostringstream text;
      text << "cannot find object \"" << object_id << '@' << class_name << '\"';
      ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
    }
  }
  else {
    std::ostringstream text;
    text << "cannot find class \"" << class_name << '\'';
    ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
  }
  
  return 0;
}


void
OksDataEditorMainDialog::show_references(const G_Object *g_obj)
{
  for(std::list<G_Window *>::iterator i = mainDlg->p_windows.begin(); i != mainDlg->p_windows.end(); ++i)
    if((*i)->show_references(g_obj)) break;
}


void
OksDataEditorMainDialog::show_relationships(const G_Object *g_obj)
{
  for(std::list<G_Window *>::iterator i = mainDlg->p_windows.begin(); i != mainDlg->p_windows.end(); ++i)
    if((*i)->show_relationships(g_obj)) break;
}


void
OksDataEditorMainDialog::update_object_id(const OksObject * oks_obj)
{
  for(std::list<G_Window *>::iterator i = mainDlg->p_windows.begin(); i != mainDlg->p_windows.end(); ++i)
    (*i)->update_object_id(oks_obj);
}
