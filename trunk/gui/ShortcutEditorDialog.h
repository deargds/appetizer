﻿/*
  Copyright (C) 2008 Laurent Cozic. All right reserved.
  Use of this source code is governed by a GNU/GPL license that can be
  found in the LICENSE file.
*/

#include "../stdafx.h"

#ifndef __ShortcutEditorDialogBase_H
#define __ShortcutEditorDialogBase_H


#include "ShortcutEditorDialogBase.h"
#include "../FolderItem.h"


class ShortcutEditorDialog: public ShortcutEditorDialogBase {

public:

  ShortcutEditorDialog(wxWindow* parent);
  void LoadFolderItem(appFolderItem* folderItem);
  void Localize();

private:

  appFolderItem* folderItem_;
  wxString selectedIconPath_;
  int selectedIconIndex_;

  void UpdateFromFolderItem();
  void UpdateFolderItemIconFields();
  void EnableDisableFields();

  void OnSaveButtonClick(wxCommandEvent& evt);
  void OnBrowseButtonClick(wxCommandEvent& evt);
  void OnChangeIconButtonClick(wxCommandEvent& evt);
  void OnUseDefaultIconButtonClick(wxCommandEvent& evt);

  DECLARE_EVENT_TABLE();

};


#endif // __ShortcutEditorDialogBase_H