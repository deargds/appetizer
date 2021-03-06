/*
  Copyright (C) 2008 Laurent Cozic. All right reserved.
  Use of this source code is governed by a GNU/GPL license that can be
  found in the LICENSE file.
*/

#include "ConfigDialog.h"
#include "../Localization.h"
#include "../FilePaths.h"
#include "../Controller.h"
#include "../UserSettings.h"
#include "../MainFrame.h"
#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/clntdata.h>
#include <wx/filename.h>


extern Controller gController;
extern MainFrame* gMainFrame;


BEGIN_EVENT_TABLE(ConfigDialog, wxDialog)
  EVT_BUTTON(ID_CDLG_BUTTON_Cancel, ConfigDialog::OnCancelButtonClick)
  EVT_BUTTON(ID_CDLG_BUTTON_Save, ConfigDialog::OnSaveButtonClick)
  EVT_BUTTON(ID_CDLG_BUTTON_CheckForUpdate, ConfigDialog::OnCheckForUpdateButtonClick)  
  EVT_SHOW(ConfigDialog::OnShow)
END_EVENT_TABLE()


ConfigDialog::ConfigDialog()
: ConfigDialogBase(NULL, wxID_ANY, wxEmptyString) {
  languageComboBox->SetMinSize(wxSize(0, languageComboBox->GetMinSize().GetHeight()));
  iconSizeComboBox->SetMinSize(wxSize(0, iconSizeComboBox->GetMinSize().GetHeight()));
  
  Localize();
}


void ConfigDialog::OnShow(wxShowEvent& evt) {
  // Update "General" panel
  languageLabel->GetParent()->Layout();
  // Update "Appearance" panel
  skinLabel->GetParent()->Layout();
}


void ConfigDialog::Localize() {
  SetTitle(LOC(_T("ConfigDialog.Title")));  
  notebook->SetPageText(0, LOC(_T("ConfigDialog.GeneralTab")));
  notebook->SetPageText(1, LOC(_T("ConfigDialog.AppearanceTab")));
  languageLabel->SetLabel(LOC(_T("ConfigDialog.LanguageLabel")));
  iconSizeLabel->SetLabel(LOC(_T("ConfigDialog.IconSizeLabel")));
  skinLabel->SetLabel(LOC(_T("ConfigDialog.SkinLabel")));
  saveButton->SetLabel(LOC(_T("Global.Save")));
  cancelButton->SetLabel(LOC(_T("Global.Cancel")));
  orientationLabel->SetLabel(LOC(_T("ConfigDialog.Orientation")));
  checkForUpdateButton->SetLabel(LOC(_T("ConfigDialog.UpdateButton")));
}


void ConfigDialog::LoadSettings() {
  UserSettingsSP userSettings = gController.GetUser()->GetSettings();

  //***************************************************************************
  // Populate language dropdown list
  //***************************************************************************

  wxString localeFolderPath = FilePaths::GetLocalesDirectory();

  wxArrayString foundFilePaths;
  wxDir localeFolder;

  // Get all the text files in the locale folder
  localeFolder.GetAllFiles(localeFolderPath, &foundFilePaths, _T("*.txt"));

  languageComboBox->Clear();

  int selectedIndex = 0;
  wxString currentLocaleCode = userSettings->Locale;

  for (int i = 0; i < foundFilePaths.Count(); i++) {
    wxString filePath = foundFilePaths[i];

    // Get the locale code from the filename
    wxFileName filename(filePath);
    filename.ClearExt();
    wxString localeCode = filename.GetName();

    // Get the language name from the file
    wxString languageName = Localization::GetLanguageName(filePath);
    wxStringClientData* clientData = new wxStringClientData(localeCode);

    if (localeCode == currentLocaleCode) selectedIndex = i;
    languageComboBox->Append(languageName, clientData);
  }

  languageComboBox->Select(selectedIndex);

  //***************************************************************************
  // Populate "icon size" dropdown list
  //***************************************************************************

  iconSizeComboBox->Clear();
  iconSizeComboBox->Append(LOC(_T("Icon.Size16")), new wxStringClientData(_T("16")));
  iconSizeComboBox->Append(LOC(_T("Icon.Size32")), new wxStringClientData(_T("32")));

  if (userSettings->IconSize == 16) {
    iconSizeComboBox->Select(0);
  } else {
    iconSizeComboBox->Select(1);
  }

  //***************************************************************************
  // Populate "Orientation" dropdown list
  //***************************************************************************

  orientationComboBox->Clear();
  orientationComboBox->Append(LOC(_T("ConfigDialog.HorizontalOrientation")), new wxStringClientData(_T("h")));
  orientationComboBox->Append(LOC(_T("ConfigDialog.VerticalOrientation")), new wxStringClientData(_T("v")));
  orientationComboBox->Select(userSettings->Rotated ? 1 : 0);

  //***************************************************************************
  // Populate "Skin" dropdown list
  //***************************************************************************

  skinComboBox->Clear();
  wxString skinFolderPath = FilePaths::GetBaseSkinDirectory();

  foundFilePaths.Clear();
  wxDir skinFolder;

  if (wxFileName::DirExists(skinFolderPath) && skinFolder.Open(skinFolderPath)) {
    wxString folderName;
    bool success = skinFolder.GetFirst(&folderName, wxALL_FILES_PATTERN, wxDIR_DIRS);
    int i = 0;

    while (success) {
      skinComboBox->Append(folderName, new wxStringClientData(folderName));
      success = skinFolder.GetNext(&folderName);
      if (folderName == userSettings->Skin) selectedIndex = i;
      i++;
    }
  } 

  skinComboBox->Select(selectedIndex);
}


void ConfigDialog::OnCheckForUpdateButtonClick(wxCommandEvent& evt) {
  checkForUpdateButton->SetLabel(LOC(_T("ConfigDialog.UpdateButtonWait")));
  checkForUpdateButton->Disable();
  checkForUpdateButton->Update();
  gController.CheckForNewVersion(false);
  checkForUpdateButton->Enable();
  checkForUpdateButton->SetLabel(LOC(_T("ConfigDialog.UpdateButton")));
}


void ConfigDialog::OnCancelButtonClick(wxCommandEvent& evt) {
  EndDialog(wxID_CANCEL);
}


void ConfigDialog::OnSaveButtonClick(wxCommandEvent& evt) {
  UserSettingsSP userSettings = gController.GetUser()->GetSettings();
  wxStringClientData* clientData;
  
  //***************************************************************************
  // Apply changes to locale code
  //***************************************************************************

  clientData = (wxStringClientData*)(languageComboBox->GetClientObject(languageComboBox->GetSelection()));
  wxString localeCode = clientData->GetData();
  
  if (localeCode != userSettings->Locale) {
    userSettings->Locale = localeCode;
    Localization::Instance->LoadLocale(localeCode, FilePaths::GetLocalesDirectory());
    Localization::Instance->SetCurrentLocale(localeCode);
    gController.User_LocaleChange();
  }

  //***************************************************************************
  // Apply changes to icon size
  //***************************************************************************
  clientData = (wxStringClientData*)(iconSizeComboBox->GetClientObject(iconSizeComboBox->GetSelection()));
  wxString newIconSizeS = clientData->GetData();
  long newIconSize; 
  newIconSizeS.ToLong(&newIconSize);

  if (newIconSize != userSettings->IconSize) {
    userSettings->IconSize = newIconSize;
    gController.User_IconSizeChange();
  }

  //***************************************************************************
  // Apply changes to orientation
  //***************************************************************************
  clientData = (wxStringClientData*)(orientationComboBox->GetClientObject(orientationComboBox->GetSelection()));
  bool rotated = clientData->GetData() == _T("v");

  if (rotated != userSettings->Rotated) {
    userSettings->Rotated = rotated;
    gMainFrame->SetRotated(rotated, true);
  }

  //***************************************************************************
  // Apply changes to skin
  //***************************************************************************
  clientData = (wxStringClientData*)(skinComboBox->GetClientObject(skinComboBox->GetSelection()));
  wxString skinName = clientData->GetData();

  if (skinName != userSettings->Skin) {
    userSettings->Skin = skinName;
    gMainFrame->ApplySkin(skinName);
  }

  gController.GetUser()->Save(true);

  EndDialog(wxID_OK);
}