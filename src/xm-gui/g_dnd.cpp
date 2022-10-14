#include <strings.h>
#include <iostream>

#include "g_dnd.h"

const char * OksObjectDragAndDropInfo::atom_oks_object_name = "_OKS_OBJECT";
const char * OksObjectDragAndDropInfo::atom_oks_object_info_name = "_OKS_OBJECT_INFO";


OksObjectDragAndDropInfo *
OksObjectDragAndDropInfo::create(const char *id, const char *name, Widget widget)
{
  unsigned long length = sizeof(OksObjectDragAndDropInfo) + 2;

  length += strlen(id) + strlen(name);

  OksObjectDragAndDropInfo *ptr = (OksObjectDragAndDropInfo *)XtMalloc(length);


  char *object_id = (char *)ptr + sizeof(OksObjectDragAndDropInfo);
  char *class_name = object_id + 1 + strlen(id);


  ptr->info_length = length;
  ptr->w = widget;

  strcpy(object_id, id);
  strcpy(class_name, name);

  return ptr;
}

std::ostream&
operator<<(std::ostream& s, const OksObjectDragAndDropInfo& o)
{
  s << "DragAndDropInfo for \'" << (void *)(&o) << "\'\n"
    << " - object id: \'" << o.get_object_id() << "\'\n"
    << " - class name: \'" << o.get_class_name() << "\'\n"
    << " - widget: \'" << (void *)(o.w) << "\'\n"
    << " - info length: " << o.info_length << std::endl;
 
  return s;
}
