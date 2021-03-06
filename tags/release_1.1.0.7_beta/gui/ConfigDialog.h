﻿/*
  Copyright (C) 2008 Laurent Cozic. All right reserved.
  Use of this source code is governed by a GNU/GPL license that can be
  found in the LICENSE file.
*/

#include "../stdafx.h"

#ifndef __ConfigDialog_H
#define __ConfigDialog_H


#include "ConfigDialogBase.h"


class ConfigDialog: public ConfigDialogBase {

public:

  ConfigDialog();
  void Localize();
  void LoadSettings();
  void UpdatePage(int pageIndex);

private:

  void OnSaveButtonClick(wxCommandEvent& evt);
  void OnShow(wxShowEvent& evt);
  void OnCheckForUpdateButtonClick(wxCommandEvent& evt);
  void OnNoteBookPageChanged(wxNotebookEvent& evt);
  void OnButtonClick(wxCommandEvent& evt);

  std::vector<bool> updatedPages_;

  DECLARE_EVENT_TABLE();

};


#endif // __ConfigDialog_H