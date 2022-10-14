#include <string>

#include <oks/attribute.h>
#include <oks/class.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>

#include "schema_editor_oks_attribute.xpm"

#include "schema_editor_attribute_dlg.h"
#include "schema_editor_main_dlg.h"
#include "schema_editor.h"


static const char szOksAttribute[] = "Oks Attribute";


void
OksSchemaEditorAttributeDialog::iconCB(Widget w, XtPointer client_data, XtPointer)
{
  long user_choice = (long)OksXm::get_user_data(w);

  OksPopupMenu::destroy(w);

  OksSchemaEditorAttributeDialog * dlg = (OksSchemaEditorAttributeDialog *)client_data;
  OksClass * c = dlg->get_class();
  OksAttribute * a = dlg->get_attribute();

  try {
    switch(user_choice) {
      case idUpdate:
        dlg->update();
        break;

      case idRename:
        a->set_name(
          OksXm::ask_name(
            dlg->parent()->get_form_widget(),
            "Input new name of the attribute",
            "Rename OKS Attribute",
	    a->get_name().c_str()
          )
        );
        dlg->set_value(idName, a->get_name().c_str());
        break;

      case idDelete:
        delete dlg;
        c->remove(a);
        break;
    }
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorAttributeDialog::iconCB") << ex.what() << std::endl;
  }
}


void
OksSchemaEditorAttributeDialog::iconAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  make_popup(
    w, client_data, event,
    (((OksSchemaEditorAttributeDialog *)client_data)->is_read_only()),
    iconCB
  );
}


OksSchemaEditorAttributeDialog::OksSchemaEditorAttributeDialog(OksAttribute *at, OksClass *cl, OksSchemaEditorMainDialog *p) :
  OksSchemaEditorDialog	(p, cl),
  a			(at)
{
  static Pixmap	pixmap = 0;

  if(!pixmap)
    pixmap = OksXm::create_color_pixmap(get_form_widget(), schema_editor_oks_attribute_xpm);
  
  createOksDlgHeader(&pixmap, iconAC, (void *)this, idName, szName, idDefinedInClass, szDefinedInClass);

  setIcon(pixmap);


    // set name and class

  set_value(idName, a->get_name().c_str());
  set_value(idDefinedInClass, get_class()->get_name().c_str());
  OksXm::TextField::set_not_editable(get_widget(idName));
  OksXm::TextField::set_not_editable(get_widget(idDefinedInClass));


    // add built-in types

  std::list<const char *> types;
  types.push_back(OksAttribute::bool_type);
  types.push_back(OksAttribute::s8_int_type);
  types.push_back(OksAttribute::u8_int_type);
  types.push_back(OksAttribute::s16_int_type);
  types.push_back(OksAttribute::u16_int_type);
  types.push_back(OksAttribute::s32_int_type);
  types.push_back(OksAttribute::u32_int_type);
  types.push_back(OksAttribute::s64_int_type);
  types.push_back(OksAttribute::u64_int_type);
  types.push_back(OksAttribute::float_type);
  types.push_back(OksAttribute::double_type);
  types.push_back(OksAttribute::date_type);
  types.push_back(OksAttribute::time_type);
  types.push_back(OksAttribute::string_type);
  types.push_back(OksAttribute::enum_type);
  types.push_back(OksAttribute::class_type);
  types.push_back(szType);
  
  add_option_menu(idTypeList, &types);
  set_value(idTypeList, a->get_type().c_str());
  set_option_menu_cb(idTypeList, typeCB, (XtPointer)this);


    // add formats

  std::list<const char *> formats;
  formats.push_back(OksAttribute::format2str(OksAttribute::Dec));
  formats.push_back(OksAttribute::format2str(OksAttribute::Hex));
  formats.push_back(OksAttribute::format2str(OksAttribute::Oct));
  formats.push_back("Formats");
  
  add_option_menu(idFormats, &formats);
  set_value(idFormats, OksAttribute::format2str(a->get_format()));

    // add cardinality

  std::list<const char *> cardinalityList;
  cardinalityList.push_back(szSingleValue);
  cardinalityList.push_back(szMultiValue);
  cardinalityList.push_back(szCardinality);

  add_option_menu(idCardinality, &cardinalityList);
  set_value(idCardinality, (a->get_is_multi_values() ? szMultiValue : szSingleValue));

  attach_previous(idFormats);

    // add non-null

  std::list<const char *> nonNullValueList;
  nonNullValueList.push_back(szCanBeNull);
  nonNullValueList.push_back(szNonNull);
  nonNullValueList.push_back(szNonNullValue);

  add_option_menu(idNonNullValue, &nonNullValueList);
  set_value(idNonNullValue, (a->get_is_no_null() ? szNonNull : szCanBeNull));


    // add range

  add_text_field(idRange, szRange);
  attach_right(idRange);
  set_value(idRange, a->get_range().c_str());


    // add initial value

  add_text_field(idInitialValue, szInitialValue);
  attach_right(idInitialValue);
  set_value(idInitialValue, a->get_init_value().c_str());


    // add description
 
  add_text(idDescription, szDescription);
  set_value(idDescription, a->get_description().c_str());

  attach_right(idDescription);


  add_separator();
  
  XtAddCallback(addCloseButton("OK"), XmNactivateCallback, closeCB, (XtPointer)this);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);
  
  attach_bottom(idDescription);

  change_schema_file();
  show();
  
  setMinSize();
  setCascadePos();

  typeCB(0, this, 0);
}

void
OksSchemaEditorAttributeDialog::typeCB(Widget, XtPointer client_data, XtPointer)
{
  OksSchemaEditorAttributeDialog *dlg = (OksSchemaEditorAttributeDialog *)client_data;

  char *value = dlg->get_value(idTypeList);

  if(
   strcmp(value, OksAttribute::s8_int_type)  &&
   strcmp(value, OksAttribute::u8_int_type)  &&
   strcmp(value, OksAttribute::s16_int_type) &&
   strcmp(value, OksAttribute::u16_int_type) &&
   strcmp(value, OksAttribute::s32_int_type)  &&
   strcmp(value, OksAttribute::u32_int_type)  &&
   strcmp(value, OksAttribute::s64_int_type) &&
   strcmp(value, OksAttribute::u64_int_type)
  )
    dlg->hide_w(idFormats);
  else
    dlg->show_w(idFormats);

  Widget w = dlg->get_widget(idRange);

  if(
   strcmp(value, OksAttribute::bool_type)
  ) {
    OksXm::TextField::set_editable(w);
    dlg->show_w(idRange);
    dlg->show_w(idNonNullValue);
  }
  else {
    OksXm::TextField::set_not_editable(w);
    dlg->hide_w(idRange);
    dlg->hide_w(idNonNullValue);
  }
}


void
OksSchemaEditorAttributeDialog::update()
{
  try {

      // set type (note that enum type is updated via callback)
  
    a->set_type(get_value(idTypeList), true);


      // set cardinality

    a->set_is_multi_values(strcmp(get_value(idCardinality), szMultiValue) ? false : true);


      // set format

    if(a->is_integer()) {
      a->set_format(OksAttribute::str2format(get_value(idFormats)));
    }


      // check range

    {
      OksXm::AutoCstring value(XmTextGetString(get_widget(idRange)));
      a->set_range(value.get());
      std::string s = a->get_range();
      if(s != value.get()) XmTextSetString(get_widget(idRange), const_cast<char *>(s.c_str()));
    }


      // check initial value

    {
      OksXm::AutoCstring value(XmTextGetString(get_widget(idInitialValue)));
      a->set_init_value(value.get());
    }


      // check description

    {
      OksXm::AutoCstring value(XmTextGetString(get_widget(idDescription)));
      a->set_description(value.get());
    }


      // set no-null

    a->set_is_no_null(strcmp(get_value(idNonNullValue), szNonNull) ? false : true);

  }
  catch(std::exception& ex) {
    Oks::error_msg("OksSchemaEditorAttributeDialog::update()") << ex.what() << std::endl;
  }
}


void
OksSchemaEditorAttributeDialog::change_schema_file()
{
    // set title

  {
    std::string s(a->get_name().c_str());
    if(is_read_only()) s += " (read-only)";
    setTitle(szOksAttribute, get_class()->get_name().c_str(), s.c_str());
  }


    // set access to the attribute properties

  {
    bool state = is_read_only() ? false : true;

    XtSetSensitive(XmOptionButtonGadget(get_widget(idTypeList)),     state);
    XtSetSensitive(XmOptionButtonGadget(get_widget(idNonNullValue)), state);
    XtSetSensitive(XmOptionButtonGadget(get_widget(idCardinality)),  state);
    XtSetSensitive(XmOptionButtonGadget(get_widget(idFormats)),      state);

    if(is_read_only()) {
      OksXm::TextField::set_not_editable(get_widget(idRange));
      OksXm::TextField::set_not_editable(get_widget(idInitialValue));
      OksXm::TextField::set_not_editable(get_widget(idDescription));
    }
    else {
      OksXm::TextField::set_editable(get_widget(idRange));
      OksXm::TextField::set_editable(get_widget(idInitialValue));
      OksXm::TextField::set_editable(get_widget(idDescription));
    }
  }
}


OksSchemaEditorAttributeDialog::~OksSchemaEditorAttributeDialog()
{
  update();
  parent()->remove_dlg(this);
}
