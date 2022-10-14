#include <ctime>
#include <sstream>

#include <ers/ers.h>

#include <Xm/Matrix.h>

#include <oks/object.h>

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "data_editor_repository_versions_dlg.h"
#include "data_editor_merge_conflicts_dlg.h"

OksDataEditorRepositoryVersionsDialog::OksDataEditorRepositoryVersionsDialog(
  std::vector<OksRepositoryVersion> versions, OksDataEditorMainDialog *p, bool archival
) : OksDataEditorDialog (p), m_versions(versions), m_archival(archival)
{
  const char * title = (m_archival
    ? "Archived Versions"
    : "Update Local Repository"
  );

  const char * label = (m_archival
    ? "Select an archival version to load."
    : "There are new repository versions. Updating your repository with latest version is required for any commit."
  );

  setTitle( title );
  setIcon((OksDialog *)parent());

  m_table = add_matrix(idTable, (void *)label);
  attach_right(idTable);

  {
    String labels[5];

    labels[0] = (char *)"Versions";
    labels[1] = (char *)"Date";
    labels[2] = (char *)"Who";
    labels[3] = (char *)"Comment";
    labels[4] = (char *)"Files";

    unsigned char label_alinments[5];
    short widths[5];

    for(size_t i=0; i<5; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }

    XtVaSetValues(
      m_table,
      XmNcolumns, 5,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      /*XmNmultiLineCell, true,*/
      XmNfill, true,
      NULL
    );
  }

  XtAddCallback(m_table, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
  XtAddCallback(m_table, XmNlabelActivateCallback, labelCB, 0);

  if (m_archival)
    {
      XtAddCallback(m_table, XmNdefaultActionCallback, versionCB, reinterpret_cast<XtPointer>(this));
      XtAddCallback(add_push_button(idCancel, "Close"), XmNactivateCallback, closeCB, (XtPointer)this);
    }
  else
    {
      XtAddCallback(add_push_button(idSave, "Save changes"), XmNactivateCallback, saveCB, (XtPointer)this);

      {
        Widget w = add_push_button(idDiscard, "Discard changes and update");
        XtAddCallback(w, XmNactivateCallback, discardAndReloadCB, (XtPointer)this);
        XtAddCallback(w, XmNactivateCallback, closeCB, (XtPointer)this);
        attach_previous(idDiscard);
      }

      {
        Widget w = add_push_button(idMerge, "Merge changes and update");
        XtAddCallback(w, XmNactivateCallback, mergeAndReloadCB, (XtPointer)this);
        XtAddCallback(w, XmNactivateCallback, closeCB, (XtPointer)this);
        attach_previous(idMerge);
      }

      {
        Widget w = add_push_button(idReload, "Update");
        XtAddCallback(w, XmNactivateCallback, reloadCB, (XtPointer)this);
        XtAddCallback(w, XmNactivateCallback, closeCB, (XtPointer)this);
        attach_previous(idReload);
      }

      XtAddCallback(add_push_button(idCancel, "Take decision later"), XmNactivateCallback, closeCB, (XtPointer)this);
      attach_previous(idCancel);
    }

  attach_bottom(idTable);

  refresh();
  show();

  XtAddCallback(m_table, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  setCascadePos();
}

void
OksDataEditorRepositoryVersionsDialog::refresh()
{
  const int num_of_rows(m_versions.size());

  String ** cells = new String * [ num_of_rows ];
  char ** dates = new char  * [ num_of_rows ];

  std::vector<std::string> files;
  files.reserve(num_of_rows);

  int idx(0);

  for (const auto& x : m_versions)
    {
      cells[idx] = new String [5];
      cells[idx][0] = const_cast<String>(x.m_commit_hash.c_str());

      dates[idx] = new char [50];
      std::strftime(dates[idx], 50, "%F %T %Z", std::localtime(&x.m_date));

      cells[idx][1] = const_cast<String>(dates[idx]);
      cells[idx][2] = const_cast<String>(x.m_user.c_str());
      cells[idx][3] = const_cast<String>(x.m_comment.c_str());

      std::string list_of_files;

      for(const auto& f : x.m_files)
        {
          if(!list_of_files.empty())
            list_of_files.push_back(' ');

          list_of_files.append(f);
        }

      files.push_back(list_of_files);

      cells[idx][4] = const_cast<String>(files.back().c_str());

      idx++;
    }

  XtVaSetValues(m_table, XmNrows, num_of_rows, NULL);
  XtVaSetValues(m_table, XmNcells, cells, NULL);

  idx = 0;

  while (idx < num_of_rows)
    {
      delete [] cells[idx];
      delete [] dates[idx++];
    }

  delete [] cells;
  delete [] dates;

  OksXm::Matrix::adjust_column_widths(m_table);
  OksXm::Matrix::set_visible_height(m_table);
  OksXm::Matrix::set_cell_background(m_table, parent()->schema_list);
  OksXm::Matrix::set_widths(m_table, 5);
}


OksDataEditorRepositoryVersionsDialog::~OksDataEditorRepositoryVersionsDialog()
{
  if (m_archival)
    parent()->hide_archived_repository_versions_dlg();
  else
    parent()->hide_new_repository_versions_dlg();
}


void OksDataEditorRepositoryVersionsDialog::add(const std::vector<OksRepositoryVersion>& versions)
{


}

void OksDataEditorRepositoryVersionsDialog::replace(std::vector<OksRepositoryVersion> versions)
{
  m_versions = versions;
  refresh();
}

void OksDataEditorRepositoryVersionsDialog::reload(OksKernel::RepositoryUpdateType type, const std::string& hash_val)
{
  OksDataEditorMainDialog * main_dlg = parent();

  try
    {
      if (hash_val.empty())
        main_dlg->update_repository("", "", type);
      else
        main_dlg->update_repository(hash_val, type);
    }
  catch(std::exception& ex)
    {
      std::cerr << "Failed to update repository: " << ex.what() << std::endl;
    }


  try
    {
      std::list<std::string> unmerged = main_dlg->get_repository_unmerged_files();
      if (!unmerged.empty())
        new OksDataEditorMergeConflictsDialog(main_dlg, unmerged);
    }
  catch(std::exception& ex)
    {
      std::cerr << "Failed to update repository: " << ex.what() << std::endl;
    }

  bool restart_kernel = false;

  std::list<OksFile *> * mfiles = nullptr;
  std::list<OksFile *> * rfiles = nullptr;

  main_dlg->create_lists_of_updated_schema_files(&mfiles, &rfiles);

  if (rfiles)
    {
      std::cout << "removed schema files:\n";
      for (const auto& x : *rfiles)
        std::cout << x->get_full_file_name() << std::endl;
    }

  if (mfiles)
    {
      std::cout << "modified schema files:\n";
      for (const auto& x : *mfiles)
        std::cout << x->get_full_file_name() << std::endl;
    }

  if (rfiles || mfiles)
    {
      std::ostringstream text;
      text << "New version contains schema modification incompatible with reload function including:\n";

      if (rfiles)
        {
          text << "  deleted schema files:\n";
          for (const auto& x : *rfiles)
            text << "    - \"" << x->get_full_file_name() << "\"\n";
        }

      if (mfiles)
        {
          text << "  modified schema files:\n";
          for (const auto& x : *mfiles)
            text << "    - \"" << x->get_full_file_name() << "\"\n";
        }

      restart_kernel = true;

      text << "OKS kernel will be restarted.\n";

      delete mfiles;
      delete rfiles;

      Oks::warning_msg("Reload") << text.str().c_str();
    }

  main_dlg->create_lists_of_updated_data_files(&mfiles, &rfiles);

  if (restart_kernel)
    {
      std::list<std::string> restart_data;
      std::set<std::string> removed;

      if (rfiles)
        for (const auto& x : *rfiles)
          removed.insert(x->get_full_file_name());

      if (mfiles)
        for (const auto& x : main_dlg->data_files())
          if (removed.find(*x.first) == removed.end())
            if (x.second->get_parent() == nullptr)
              restart_data.push_back(*x.first);

      if (restart_data.empty())
        {
          Oks::warning_msg("Reload") << "no top-level files are found in requested versions, load any remained files";

          for (const auto& x : main_dlg->data_files())
            if (removed.find(*x.first) == removed.end())
              restart_data.push_back(*x.first);
        }

      // destroy all dialogs
      OksXm::destroy_modal_dialogs();
      for (std::list<OksDataEditorDialog *>::iterator i = main_dlg->dialogs.begin(); i != main_dlg->dialogs.end();)
        {
          OksDataEditorDialog *oded = *i;
          i = main_dlg->dialogs.erase(i);
          delete oded;
        }

      main_dlg->unsubscribe();
      main_dlg->p_skip_menu_update = true;

      main_dlg->close_all_data();
      main_dlg->close_all_schema();

      for (const auto& x : restart_data)
        main_dlg->load_data(x);

      main_dlg->refresh_schemes();
      main_dlg->refresh_data();
      main_dlg->refresh_classes();

      main_dlg->subscribe();
      main_dlg->p_skip_menu_update = false;
    }
  else
    {
      if (rfiles)
        {
          // close removed top-level files, if any
          for(auto& x : *rfiles)
            if(x->get_parent() == nullptr)
              main_dlg->close_data(x);

          while (!rfiles->empty())
            {
              OksFile * f = rfiles->front();
              rfiles->pop_front();
              f->update_status_of_file(true, false);
            }
        }

      if (rfiles || mfiles)
        main_dlg->reload(nullptr, mfiles);
    }

  delete mfiles;
  delete rfiles;

}

void
OksDataEditorRepositoryVersionsDialog::mergeAndReloadCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorRepositoryVersionsDialog * dlg(reinterpret_cast<OksDataEditorRepositoryVersionsDialog *>(client_data));
  dlg->reload(OksKernel::MergeChanges, "");
}

void
OksDataEditorRepositoryVersionsDialog::discardAndReloadCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorRepositoryVersionsDialog * dlg(reinterpret_cast<OksDataEditorRepositoryVersionsDialog *>(client_data));
  dlg->reload(OksKernel::DiscardChanges, "");
}

void
OksDataEditorRepositoryVersionsDialog::reloadCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorRepositoryVersionsDialog * dlg(reinterpret_cast<OksDataEditorRepositoryVersionsDialog *>(client_data));
  dlg->reload(OksKernel::NoChanges, "");
}

void
OksDataEditorRepositoryVersionsDialog::versionCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksDataEditorRepositoryVersionsDialog * dlg(reinterpret_cast<OksDataEditorRepositoryVersionsDialog *>(client_data));
  XbaeMatrixDefaultActionCallbackStruct * cb(reinterpret_cast<XbaeMatrixDefaultActionCallbackStruct *>(call_data));

  char *version = XbaeMatrixGetCell(dlg->m_table, cb->row, 0);

  dlg->reload(OksKernel::NoChanges, version);

  delete dlg;
}

void
OksDataEditorRepositoryVersionsDialog::labelCB(Widget w, XtPointer /*d*/, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct * cbs(reinterpret_cast<XbaeMatrixLabelActivateCallbackStruct *>(cb));

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, OksXm::Matrix::SortByString);
}

void
OksDataEditorRepositoryVersionsDialog::saveCB(Widget w, XtPointer d, XtPointer cb)
{
  OksDataEditorRepositoryVersionsDialog * dlg(reinterpret_cast<OksDataEditorRepositoryVersionsDialog *>(d));

  for (auto& f : dlg->parent()->data_files())
    {
      if (f.second->is_updated())
        {
          try
            {
              dlg->parent()->save_data(f.second, true);
            }
          catch(std::exception& ex)
            {
              std::cerr << "Failed to save file \"" << f.first << "\': " << ex.what() << std::endl;
            }
        }
    }
}

void
OksDataEditorRepositoryVersionsDialog::closeCB(Widget, XtPointer d, XtPointer)
{
  delete reinterpret_cast<OksDataEditorRepositoryVersionsDialog *>(d);
}
