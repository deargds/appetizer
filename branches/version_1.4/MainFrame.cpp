﻿/*
  Copyright (C) 2008 Laurent Cozic. All right reserved.
  Use of this source code is governed by a GNU/GPL license that can be
  found in the LICENSE file.
*/

#include "stdafx.h"

#include "MainFrame.h"
#include "Constants.h"
#include "MiniLaunchBar.h"
#include "MessageBoxes.h"
#include "FilePaths.h"
#include "Styles.h"
#include "Localization.h"
#include "utilities/XmlUtil.h"
#include "utilities/Updater.h"
#include "utilities/VersionInfo.h"
#include "gui/AboutDialog.h"
#include "gui/ImportWizardDialog.h"
#include "bitmap_controls/ImageButton.h"


BEGIN_EVENT_TABLE(MainFrame, wxFrame)
  EVT_SIZE(MainFrame::OnSize)
  EVT_MOVE(MainFrame::OnMove)
  EVT_ERASE_BACKGROUND(MainFrame::OnEraseBackground)
  EVT_PAINT(MainFrame::OnPaint)
  EVT_CLOSE(MainFrame::OnClose)
  EVT_COMMAND(wxID_ANY, wxeEVT_CLICK, MainFrame::OnImageButtonClick)
  EVT_IDLE(MainFrame::OnIdle)
  EVT_MOUSE_CAPTURE_LOST(MainFrame::OnMouseCaptureLost)
  EVT_HOTKEY(HOT_KEY_ID, MainFrame::OnHotKey)
  EVT_ACTIVATE(MainFrame::OnActivate)
  EVT_HELP(wxID_ANY, MainFrame::OnHelp)
END_EVENT_TABLE()


//#include "launchapp/Launchapp.h"
//Launchapp* launchapp_;


MainFrame::MainFrame()
: wxFrame(
  (wxFrame *)NULL,
  wxID_ANY,
  wxEmptyString,
  wxDefaultPosition,
  wxDefaultSize,
  0 | wxFRAME_SHAPED | wxNO_BORDER | (wxGetApp().GetUser()->GetSettings()->GetBool(_T("TaskBarIcon")) ? 0 : wxFRAME_NO_TASKBAR) | (wxGetApp().GetUser()->GetSettings()->GetBool(_T("AlwaysOnTop")) ? wxSTAY_ON_TOP : 0)
  )
{  
  logWindow_ = NULL;
  aboutDialog_ = NULL;
  arrowButtonOpenIcon_ = NULL;
  arrowButtonCloseIcon_ = NULL;
  nullPanel_ = NULL;
  mainBackgroundBitmap_ = NULL;
  //launchapp_ = NULL;
  rotated_ = false;
  activated_ = false;  
  closeOperationScheduled_ = false;
  initialized_ = false;
  isClosing_ = false;
  closeStep_ = -1;

  bool showLogWindow = false;
  #ifdef __WXDEBUG__
  showLogWindow = true;
  #endif // __WXDEBUG__

  if (wxGetApp().GetCommandLine().Found(_T("l")) || showLogWindow) {
    logWindow_ = new wxLogWindow(this, wxEmptyString, true);
    wxLog::SetActiveTarget(logWindow_);
  }

  hotKeyRegistered_ = false;
  firstIdleEventSent_ = false;
  needLayoutUpdate_ = true;
  needMaskUpdate_ = true;
  optionPanelOpen_ = false;
  openCloseAnimationDockLeft_ = true;
  optionPanelOpenWidth_ = 0;
  optionPanelMaxOpenWidth_ = 50;
  openCloseAnimationDuration_ = 50;

  // Load the mask and background images
  maskNineSlices_.LoadImage(FilePaths::GetSkinDirectory() + _T("/MainBackground.png"), false);

  windowDragData_.DraggingStarted = false;

  nullPanel_ = new wxPanel(this, 0, 0, 0, 0); 

  arrowButton_ = new ImageButton(this, ID_BUTTON_Arrow, wxPoint(0, 0), wxSize(10, 10));
  arrowButton_->SetName(_T("ArrowButton"));
  arrowButton_->SetCursor(wxCursor(wxCURSOR_HAND));

  backgroundPanel_ = new NineSlicesPanel(this, wxID_ANY, wxPoint(0,0), wxSize(50,50));
  backgroundPanel_->SetName(_T("BackgroundPanel"));
  backgroundPanel_->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(MainFrame::OnMouseDown), NULL, this);
  backgroundPanel_->Connect(wxID_ANY, wxEVT_LEFT_UP, wxMouseEventHandler(MainFrame::OnMouseUp), NULL, this);
  backgroundPanel_->Connect(wxID_ANY, wxEVT_MOTION, wxMouseEventHandler(MainFrame::OnMouseMove), NULL, this);

  optionPanel_ = new OptionPanel(this);

  resizerPanel_ = new ImagePanel(backgroundPanel_, wxID_ANY, wxPoint(0, 0), wxSize(50, 50));
  resizerPanel_->SetCursor(wxCursor(wxCURSOR_SIZENWSE));
  resizerPanel_->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(MainFrame::OnResizerMouseDown), NULL, this);
  resizerPanel_->Connect(wxID_ANY, wxEVT_LEFT_UP, wxMouseEventHandler(MainFrame::OnResizerMouseUp), NULL, this);
  resizerPanel_->Connect(wxID_ANY, wxEVT_MOTION, wxMouseEventHandler(MainFrame::OnResizerMouseMove), NULL, this);

  iconPanel_ = new IconPanel(backgroundPanel_, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
  iconPanel_->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(MainFrame::OnMouseDown), NULL, this);
  iconPanel_->Connect(wxID_ANY, wxEVT_LEFT_UP, wxMouseEventHandler(MainFrame::OnMouseUp), NULL, this);
  iconPanel_->Connect(wxID_ANY, wxEVT_MOTION, wxMouseEventHandler(MainFrame::OnMouseMove), NULL, this);

  closeSideButton_ = new ImageButton(backgroundPanel_, ID_BUTTON_MainFrame_CloseButton);
  closeSideButton_->SetCursor(wxCursor(wxCURSOR_HAND));

  #ifdef __WXDEBUG__
  bool showEjectSideButton = true;
  #else
  bool showEjectSideButton = wxGetApp().GetUtilities().IsApplicationOnPortableDrive();
  #endif // __WXDEBUG__

  if (showEjectSideButton) {
    ejectSideButton_ = new ImageButton(backgroundPanel_, ID_BUTTON_MainFrame_EjectButton);
    ejectSideButton_->SetCursor(wxCursor(wxCURSOR_HAND));
  } else {
    ejectSideButton_ = NULL;
  }

  //launchapp_ = new Launchapp(this);
  //launchapp_->SetSize(200,200,240,180);
  //launchapp_->Show();

  frameIcon_.LoadFile(FilePaths::GetBaseSkinDirectory() + _T("/Application.ico"), wxBITMAP_TYPE_ICO);
  SetIcon(frameIcon_);
  SetTitle(APPLICATION_NAME);
  if (wxGetApp().GetUser()->GetSettings()->GetBool(_T("TrayIcon"))) ShowTrayIcon();
  ApplySkin();
  RegisterHideShowHotKey();



  TiXmlDocument doc(FilePaths::GetWindowFile().mb_str());
  doc.LoadFile();

  TiXmlElement* root = doc.FirstChildElement("Window");
  if (!root) {
    WLOG(_T("MainFrame: Could not load XML. No Window element found."));

    SetSize(0, 0, MAIN_FRAME_DEFAULT_WIDTH, MAIN_FRAME_DEFAULT_HEIGHT);
    CentreOnScreen();
  } else {
    TiXmlHandle handle(root);
    int displayIndex = XmlUtil::ReadElementTextAsInt(handle, "DisplayIndex", 0);
    bool isLeftOfDisplay = XmlUtil::ReadElementTextAsBool(handle, "IsLeftOfDisplay", true);
    bool isTopOfDisplay = XmlUtil::ReadElementTextAsBool(handle, "IsTopOfDisplay", true);
    int horizontalGap = XmlUtil::ReadElementTextAsInt(handle, "HorizontalGap", 0);
    int verticalGap = XmlUtil::ReadElementTextAsInt(handle, "VerticalGap", 0);
    int width = XmlUtil::ReadElementTextAsInt(handle, "Width", MAIN_FRAME_DEFAULT_WIDTH);
    int height = XmlUtil::ReadElementTextAsInt(handle, "Height", MAIN_FRAME_DEFAULT_HEIGHT);

    if (displayIndex >= wxDisplay::GetCount()) displayIndex = wxDisplay::GetCount() - 1;
    
    wxDisplay display(displayIndex);

    int x = 0;
    int y = 0;

    if (isLeftOfDisplay) {
      x = display.GetGeometry().GetLeft() + horizontalGap;
    } else {
      x = display.GetGeometry().GetRight() - horizontalGap - width;
    }

    if (isTopOfDisplay) {
      y = display.GetGeometry().GetTop() + verticalGap;
    } else {
      y = display.GetGeometry().GetBottom() - verticalGap - height;
    }

    ConvertToWindowValidCoordinates(&display, x, y, width, height);

    SetSize(x, y, width, height);
  }
}


void MainFrame::ShowTrayIcon(bool doShow) {
  if (doShow == taskBarIcon_.IsIconInstalled()) return;
  
  if (doShow) {
    // We need to provide a .ico file that only contains a 16x16 icon. If we give a .ico with
    // multiple sizes (16, 32, 48), Windows is going to use the 32x32 size and resize it to 16x16 O_o
    wxIcon trayIcon(FilePaths::GetBaseSkinDirectory() + _T("/Application16.ico"), wxBITMAP_TYPE_ICO);
    taskBarIcon_.SetIcon(trayIcon);
  } else {
    taskBarIcon_.RemoveIcon();
  }
}


wxWindow* MainFrame::GetNullPanelObjectById(int id) {
  const wxWindowList& children = nullPanel_->GetChildren();
  for (int i = 0; i < children.size(); i++) {
    wxWindow* w = children[i];
    if (w->GetId() == id) return w;
  }
  return NULL;
}


void MainFrame::OnHelp(wxHelpEvent& evt) {
  wxGetApp().GetUtilities().ShowHelpFile();
}


wxPanel* MainFrame::GetNullPanel() {
  return nullPanel_;
}


bool MainFrame::RegisterHideShowHotKey() {
  UnregisterHideShowHotKey();
  
  UserSettings* userSettings = wxGetApp().GetUser()->GetSettings();
  
  if (userSettings->GetInt(_T("HotKeyKey")) <= 0) return false;

  int modifiers = 0;
  if (userSettings->GetBool(_T("HotKeyControl"))) modifiers |= wxMOD_CONTROL;
  if (userSettings->GetBool(_T("HotKeyAlt"))) modifiers |= wxMOD_ALT;
  if (userSettings->GetBool(_T("HotKeyShift"))) modifiers |= wxMOD_SHIFT;

  if (modifiers == 0) return false;

  int success = RegisterHotKey(HOT_KEY_ID, modifiers, userSettings->GetInt(_T("HotKeyKey")));
  hotKeyRegistered_ = success;
  
  if (hotKeyRegistered_) {
    ILOG(_T("Hot key registered successfully"));
  } else {
    MessageBoxes::ShowError(_("There was an error registering the hot key. Another application may already have registered it."));
  }

  return hotKeyRegistered_;
}


void MainFrame::UnregisterHideShowHotKey() {
  if (!hotKeyRegistered_) return;

  UnregisterHotKey(HOT_KEY_ID);
  ILOG(_T("Hot key unregistered"));
  hotKeyRegistered_ = false;
}


void MainFrame::OnActivate(wxActivateEvent& evt) {
  activated_ = evt.GetActive();
}


void MainFrame::OnHotKey(wxKeyEvent& evt) {
  UserSettings* s = wxGetApp().GetUser()->GetSettings();

  if (s->GetBool(_T("TrayIcon"))) {
    if (!IsVisible()) {
      Show();
      Raise();
    } else {
      if (!activated_) {
        Raise();
      } else {
        Hide();
      }
    }
  } else {
    if (IsIconized()) {
      Maximize(false);
      Raise();
    } else {
      if (!activated_) {
        Raise();
      } else {
        Iconize();
      }
    }
  }
}


void MainFrame::DoAutoHide() {
  UserSettings* s = wxGetApp().GetUser()->GetSettings();
  if (s->GetBool(_T("AutoHideApplication"))) {
    if (s->GetBool(_T("TrayIcon"))) {
      Hide();
    } else {
      Iconize();
    }
  }
}


void MainFrame::OnIdle(wxIdleEvent& evt) {
  LuaHostTable emptyEventTable;

  if (!firstIdleEventSent_) {

    // Wait until the main loop is running
    if (!wxApp::IsMainLoopRunning()) return;

    // ***********************************************************************************
    // Complete the initialization process
    // ***********************************************************************************

    firstIdleEventSent_ = true;

    // ***********************************************************************************
    // Load the plugins
    // ***********************************************************************************

    wxGetApp().InitializePluginManager();
    
    wxGetApp().GetPluginManager()->DispatchEvent(&(wxGetApp()), _T("earlyBird"), emptyEventTable); 

    // ***********************************************************************************
    // Do auto-multilaunch
    // ***********************************************************************************

    if (wxGetApp().GetUser()->GetSettings()->GetBool(_T("RunMultiLaunchOnStartUp"))) wxGetApp().GetUtilities().DoMultiLaunch();

    // ***********************************************************************************
    // If it's the first launch, show the import dialog
    // ***********************************************************************************

    if (wxGetApp().IsFirstLaunch()) {
      wxGetApp().GetUtilities().ShowImportDialog();
    }

    // ***********************************************************************************
    // Check if a new version is available
    // ***********************************************************************************

    #ifdef __WINDOWS__
    #ifndef __WXDEBUG__ 
    ILOG(_T("Update check..."));

    wxDateTime now = wxDateTime::Now();
    // The line below doesn't work on Ubuntu
    wxDateTime nextUpdateTime = wxGetApp().GetUser()->GetSettings()->GetDateTime(_T("NextUpdateCheckTime"));
    ILOG(_T("Now is %s"), now.Format());
    ILOG(_T("Next update check on %s"), nextUpdateTime.Format());

    if (!nextUpdateTime.IsLaterThan(now)) {
      wxGetApp().CheckForNewVersion(true);

      now.Add(wxTimeSpan(24 * CHECK_VERSION_DAY_INTERVAL));

      wxGetApp().GetUser()->GetSettings()->SetDateTime(_T("NextUpdateCheckTime"), now);
      wxGetApp().GetUser()->ScheduleSave();
    }
    #endif // __WXDEBUG__
    #endif //__WINDOWS__        

    initialized_ = true;

    wxGetApp().GetPluginManager()->DispatchEvent(&(wxGetApp()), _T("ready"), emptyEventTable); 
  }

  if (closeOperationScheduled_) {
    closeOperationScheduled_ = false;    
    Close();
  }

}


bool MainFrame::DoCloseStep() {

  if (closeStep_ < 0 || !isClosing_) return false;

  int step = closeStep_;

  ILOG(wxString::Format(_T("Doing close step: %d"), closeStep_));

  // Set it to -1 for now to make sure that a given step is not executed more than once
  // It needs to be set back to step + 1, once the step has been done.
  closeStep_ = -1; 

  if (step == 0) {
  
    ILOG(wxString::Format(_T("Dispatching 'close' event to plugins.")));
    LuaHostTable emptyEventTable;
    wxGetApp().GetPluginManager()->DispatchEvent(&(wxGetApp()), _T("close"), emptyEventTable); 
    
    ILOG(wxString::Format(_T("Recursively cleaning up window.")));
    RecurseCleanUp(this);  

    closeStep_ = step + 1;

  } else if (step == 1) {

    TiXmlDocument doc;
    doc.LinkEndChild(new TiXmlDeclaration("1.0", "", ""));
    TiXmlElement* xmlRoot = new TiXmlElement("Window");;
    xmlRoot->SetAttribute("version", "1.0");
    doc.LinkEndChild(xmlRoot);

    XmlUtil::AppendTextElement(xmlRoot, "DisplayIndex", GetDisplayIndex());
    XmlUtil::AppendTextElement(xmlRoot, "IsLeftOfDisplay", IsLeftOfDisplay());
    XmlUtil::AppendTextElement(xmlRoot, "IsTopOfDisplay", IsTopOfDisplay());

    if (rotated_) {
      XmlUtil::AppendTextElement(xmlRoot, "Width", GetRect().GetWidth());
      XmlUtil::AppendTextElement(xmlRoot, "Height", GetRect().GetHeight() - optionPanelOpenWidth_);
    } else {
      XmlUtil::AppendTextElement(xmlRoot, "Width", GetRect().GetWidth() - optionPanelOpenWidth_);
      XmlUtil::AppendTextElement(xmlRoot, "Height", GetRect().GetHeight());
    }
    
    int hGap;
    int vGap;

    wxDisplay display(GetDisplayIndex());

    if (IsLeftOfDisplay()) {
      hGap = GetRect().GetLeft() - display.GetGeometry().GetLeft();
    } else {    
      hGap = display.GetGeometry().GetRight() - GetRect().GetRight();
    }

    if (IsTopOfDisplay()) {
      vGap = GetRect().GetTop() - display.GetGeometry().GetTop();
    } else {    
      vGap = display.GetGeometry().GetBottom() - GetRect().GetBottom();
    }

    XmlUtil::AppendTextElement(xmlRoot, "HorizontalGap", hGap);
    XmlUtil::AppendTextElement(xmlRoot, "VerticalGap", vGap);

    FilePaths::CreateSettingsDirectory();
    doc.SaveFile(FilePaths::GetWindowFile().mb_str());

    arrowButton_->SetIcon(NULL);
    wxDELETE(arrowButtonCloseIcon_);
    wxDELETE(arrowButtonOpenIcon_);
    wxDELETE(mainBackgroundBitmap_);

    wxGetApp().CloseApplication();

    Destroy();

    closeStep_ = step + 1;
    
    return true;

  }

  ILOG(wxString::Format(_T("Next close step will be: %d"), closeStep_));

  return false;
}


void MainFrame::ApplySkin(const wxString& skinName) {
  wxString tSkinName;

  if (skinName == wxEmptyString) {
    tSkinName = wxGetApp().GetUser()->GetSettings()->GetString(_T("Skin"));
  } else {
    tSkinName = skinName;
  }


  if (!wxFileName::DirExists(FilePaths::GetSkinDirectory())) {
    WLOG(_T("Skin directory doesn't exist. Switching to default. ") + FilePaths::GetSkinDirectory());
    wxGetApp().GetUser()->GetSettings()->SetString(_T("Skin"), _T("Default"));
    tSkinName = _T("Default");
  }


  Styles::LoadSkinFile(FilePaths::GetSkinDirectory() + _T("/") + SKIN_FILE_NAME);

  wxDELETE(mainBackgroundBitmap_);

  mainBackgroundBitmap_ = new wxBitmap(FilePaths::GetSkinDirectory() + _T("/MainBackground.png"), wxBITMAP_TYPE_PNG);
  wxMemoryDC mainBackgroundDC;
  wxMemoryDC targetDC;
  mainBackgroundDC.SelectObject(*mainBackgroundBitmap_);  







  int iconPanelX = Styles::InnerPanel.SourceRectangle.GetX() - Styles::OptionPanel.SourceRectangle.GetRight();
  int iconPanelY = Styles::InnerPanel.SourceRectangle.GetY() - Styles::OptionPanel.SourceRectangle.GetTop();
  int iconPanelRightMargin = mainBackgroundBitmap_->GetWidth() - Styles::InnerPanel.SourceRectangle.GetRight();
  int iconPanelBottomMargin = mainBackgroundBitmap_->GetHeight() - Styles::InnerPanel.SourceRectangle.GetBottom();

  Styles::MainPanel.Padding.FromRect(wxRect(iconPanelX, iconPanelY, iconPanelRightMargin, iconPanelBottomMargin));








  maskNineSlices_.LoadImage(FilePaths::GetSkinDirectory() + _T("/MainBackground.png"), false);









  wxBitmap* arrowBitmapUp = new wxBitmap(Styles::ArrowButton.SourceRectangle.GetWidth(), Styles::ArrowButton.SourceRectangle.GetHeight());  
  targetDC.SelectObject(*arrowBitmapUp);
  targetDC.Blit(0, 0, arrowBitmapUp->GetWidth(), arrowBitmapUp->GetHeight(), &mainBackgroundDC, Styles::ArrowButton.SourceRectangle.GetX(), Styles::ArrowButton.SourceRectangle.GetY());  
  targetDC.SelectObject(wxNullBitmap);

  arrowButton_->LoadImages(arrowBitmapUp);
  arrowButton_->SetStateColors(wxNullColour, Styles::ArrowButton.ColorOver, Styles::ArrowButton.ColorDown);
  wxDELETE(arrowButtonOpenIcon_);
  wxDELETE(arrowButtonCloseIcon_);
  arrowButtonOpenIcon_ = new wxBitmap(FilePaths::GetSkinDirectory() + _T("/ArrowButtonIconRight.png"), wxBITMAP_TYPE_PNG);
  arrowButtonCloseIcon_ = new wxBitmap(FilePaths::GetSkinDirectory() + _T("/ArrowButtonIconLeft.png"), wxBITMAP_TYPE_PNG);
  arrowButton_->SetIcon(optionPanelOpen_ ? arrowButtonOpenIcon_ : arrowButtonCloseIcon_, false);


  resizerPanel_->SetSize(16,16);







  wxBitmap* barBackgroundBitmap = new wxBitmap(Styles::MainPanel.SourceRectangle.GetWidth(), Styles::MainPanel.SourceRectangle.GetHeight());  
  targetDC.SelectObject(*barBackgroundBitmap);
  targetDC.Blit(0, 0, barBackgroundBitmap->GetWidth(), barBackgroundBitmap->GetHeight(), &mainBackgroundDC, Styles::MainPanel.SourceRectangle.GetX(), Styles::MainPanel.SourceRectangle.GetY());  
  targetDC.SelectObject(wxNullBitmap);
  backgroundPanel_->LoadImage(barBackgroundBitmap);


  



  mainBackgroundDC.SelectObject(wxNullBitmap);





  closeSideButton_->LoadImage(FilePaths::GetSkinDirectory() + _T("/CloseButton"));
  closeSideButton_->FitToImage();   

  if (ejectSideButton_) {
    ejectSideButton_->LoadImage(FilePaths::GetSkinDirectory() + _T("/EjectButton"));
    ejectSideButton_->FitToImage();      
  }

  iconPanel_->ApplySkin(mainBackgroundBitmap_);
  optionPanel_->ApplySkin(mainBackgroundBitmap_);

  //if (launchapp_) launchapp_->ApplySkin(mainBackgroundBitmap_);

  InvalidateMask();
  InvalidateLayout();
}


wxBitmap* MainFrame::GetMainBackgroundBitmap() {
  return mainBackgroundBitmap_;
}


void MainFrame::SetRotated(bool rotated, bool swapWidthAndHeight) {
  if (rotated == rotated_) return;
  rotated_ = rotated;

  int previousWidth = GetSize().GetWidth();
  int previousHeight = GetSize().GetHeight();

  optionPanel_->SetRotated(rotated);
  iconPanel_->SetRotated(rotated);
  optionPanel_->UpdateLayout(); // We need a layout update to get a valid "required width" property, needed by ToggleOptionPanel()
  arrowButton_->SetBitmapRotation(rotated ? -90 : 0);
  backgroundPanel_->SetBitmapRotation(rotated ? -90 : 0);
  resizerPanel_->SetBitmapRotation(rotated ? -90 : 0);

  if (!rotated) {
    resizerPanel_->SetCursor(wxCursor(wxCURSOR_SIZENWSE));
  } else {
    resizerPanel_->SetCursor(wxCursor(wxCURSOR_SIZENESW));
  }
  
  UpdateLayout();
  UpdateMask();

  if (swapWidthAndHeight) {
    SetSize(previousHeight, previousWidth);    
  } else {
    SetSize(previousWidth, previousHeight);
  }

  // Need an update here to make sure that GetRect() returns the current width and height
  // (and not the previous one). Needed by ToggleOptionPanel()
  Update();
  
  // Open and close (or close and open) the option panel to force a relayout
  ToggleOptionPanel();
  ToggleOptionPanel();
}


OptionPanel* MainFrame::GetOptionPanel() {
  return optionPanel_;
}


void MainFrame::Localize() {
  if (optionPanel_) optionPanel_->Localize();
  if (closeSideButton_) closeSideButton_->SetToolTip(_("Minimize to tray"));
  if (ejectSideButton_) ejectSideButton_->SetToolTip(_("Eject drive"));
}


void MainFrame::ConvertToWindowValidCoordinates(const wxDisplay* display, int& x, int& y, int& width, int& height) {  
  int displayX = x - display->GetGeometry().GetLeft();
  int displayY = y - display->GetGeometry().GetTop();

  if (displayX + width < WINDOW_VISIBILITY_BORDER) {
    displayX = WINDOW_VISIBILITY_BORDER - width;
  } else if (displayX > display->GetGeometry().GetWidth() - WINDOW_VISIBILITY_BORDER) {
    displayX = display->GetGeometry().GetWidth() - WINDOW_VISIBILITY_BORDER;
  }

  if (displayY + height < WINDOW_VISIBILITY_BORDER) {
    displayY = WINDOW_VISIBILITY_BORDER - height;
  } else if (displayY > display->GetGeometry().GetHeight() - WINDOW_VISIBILITY_BORDER) {
    displayY = display->GetGeometry().GetHeight() - WINDOW_VISIBILITY_BORDER;
  }

  x = displayX + display->GetGeometry().GetLeft();
  y = displayY + display->GetGeometry().GetTop();
}


IconPanel* MainFrame::GetIconPanel() { return iconPanel_; }


int MainFrame::GetDisplayIndex() {
  int displayCount = wxDisplay::GetCount();
  if (displayCount <= 1) return 0;

  wxPoint centerPoint(
    GetRect().GetLeft() + floor((double)GetRect().GetWidth() / 2),
    GetRect().GetTop() + floor((double)GetRect().GetHeight() / 2));

  int index = wxDisplay::GetFromPoint(centerPoint);
  if (index < 0) return 0;
  return index;
}


bool MainFrame::IsLeftOfDisplay() {
  wxDisplay display(GetDisplayIndex());

  wxRect geometry = display.GetGeometry();
  wxPoint centerPoint(
    GetRect().GetLeft() + floor((double)GetRect().GetWidth() / 2),
    GetRect().GetTop() + floor((double)GetRect().GetHeight() / 2));

  wxRect leftRect(geometry.GetLeft(), geometry.GetTop(), geometry.GetWidth() / 2, geometry.GetHeight());

  return leftRect.Contains(centerPoint);
}


bool MainFrame::IsTopOfDisplay() {
  wxDisplay display(GetDisplayIndex());

  wxRect geometry = display.GetGeometry();
  wxPoint centerPoint(
    GetRect().GetLeft() + floor((double)GetRect().GetWidth() / 2),
    GetRect().GetTop() + floor((double)GetRect().GetHeight() / 2));

  wxRect topRect(geometry.GetLeft(), geometry.GetTop(), geometry.GetWidth(), geometry.GetHeight() / 2);

  return topRect.Contains(centerPoint);
}


void MainFrame::UpdateMask() {
  // Create the bitmap on which the 9-slices scaled mask is going to be drawn
  wxBitmap maskBitmap = wxBitmap(GetRect().GetWidth(), GetRect().GetHeight());
  
  // Create a temporary DC to do the actual drawing and assign it the bitmap
  wxMemoryDC maskDC;
  maskDC.SelectObject(maskBitmap);
  
  // Draw the nine slices on the DC
  maskNineSlices_.Draw(&maskDC, 0, 0, maskBitmap.GetWidth(), maskBitmap.GetHeight());

  // Select NULL to release the bitmap
  maskDC.SelectObject(wxNullBitmap);

  // Create the region from the bitmap and assign it to the window
  wxRegion region(maskBitmap, MASK_COLOR);
  SetShape(region);

  needMaskUpdate_ = false;
}


void MainFrame::UpdateLayout(int width, int height) {
  if (!mainBackgroundBitmap_) return;

  int sideButtonGap = 3;

  if (rotated_) { // VERTICAL ORIENTATION

    int bgPanelHeight = height - Styles::ArrowButton.SourceRectangle.GetWidth() - optionPanelOpenWidth_;

    arrowButton_->SetSize(
      0,
      bgPanelHeight + optionPanelOpenWidth_,
      width,
      Styles::ArrowButton.SourceRectangle.GetWidth());

    backgroundPanel_->SetSize(0, 0, width, bgPanelHeight);

    iconPanel_->SetSize(
      Styles::MainPanel.Padding.Bottom,
      Styles::MainPanel.Padding.Right,
      width - Styles::MainPanel.Padding.Height,
      bgPanelHeight - Styles::MainPanel.Padding.Width);

    resizerPanel_->Move(
      width - resizerPanel_->GetRect().GetWidth(),
      0);

    optionPanel_->SetSize(
      0,
      bgPanelHeight,
      width,
      optionPanelOpenWidth_);

    closeSideButton_->Move(
      Styles::InnerPanel.SourceRectangle.GetTop(),
      0);

    if (ejectSideButton_) {
      ejectSideButton_->Move(
        closeSideButton_->GetRect().GetRight() + sideButtonGap,
        closeSideButton_->GetRect().GetTop());
    }

  } else { // HORIZONTAL ORIENTATION

    arrowButton_->SetSize(
      0,
      0,
      Styles::ArrowButton.SourceRectangle.GetWidth(),
      height);
    
    int bgPanelX = Styles::ArrowButton.SourceRectangle.GetWidth() + optionPanelOpenWidth_;
    int bgPanelWidth = width - bgPanelX;

    backgroundPanel_->SetSize(bgPanelX, 0, bgPanelWidth, height);

    iconPanel_->SetSize(
      Styles::MainPanel.Padding.Left,
      Styles::MainPanel.Padding.Top,
      bgPanelWidth - Styles::MainPanel.Padding.Width,
      height - Styles::MainPanel.Padding.Height);
    
    resizerPanel_->Move(
      bgPanelWidth - resizerPanel_->GetRect().GetWidth(),
      height - resizerPanel_->GetRect().GetHeight());

    optionPanel_->SetSize(
      Styles::ArrowButton.SourceRectangle.GetWidth(),
      0,
      optionPanelOpenWidth_,
      height);

    closeSideButton_->Move(
      bgPanelWidth - closeSideButton_->GetSize().GetWidth(),
      Styles::InnerPanel.SourceRectangle.GetTop());

    if (ejectSideButton_) {
      ejectSideButton_->Move(
        closeSideButton_->GetRect().GetLeft(),
        closeSideButton_->GetRect().GetBottom() + sideButtonGap);
    }
  }
  
  needLayoutUpdate_ = false;
}


void MainFrame::UpdateLayout() {
  UpdateLayout(GetClientRect().GetWidth(), GetClientRect().GetHeight());
}


int MainFrame::GetOptionPanelTotalWidth() {
  if (rotated_) {
    return arrowButton_->GetSize().GetHeight() + optionPanelOpenWidth_;
  } else {
    return arrowButton_->GetSize().GetWidth() + optionPanelOpenWidth_;
  }
}


int MainFrame::GetMinHeight() {
  if (rotated_) {
    return iconPanel_->GetMinHeight() + GetOptionPanelTotalWidth() + Styles::MainPanel.Padding.Width;
  } else {
    return iconPanel_->GetMinHeight() + Styles::MainPanel.Padding.Height;
  }
}


int MainFrame::GetMinWidth() {
  if (rotated_) {
    return iconPanel_->GetMinWidth() + Styles::MainPanel.Padding.Height;
  } else {
    return iconPanel_->GetMinWidth() + GetOptionPanelTotalWidth() + Styles::MainPanel.Padding.Width;
  }
}


int MainFrame::GetMaxWidth() {
  if (rotated_) {
    return iconPanel_->GetMaxWidth() + Styles::MainPanel.Padding.Height;
  } else {
    return iconPanel_->GetMaxWidth() + Styles::MainPanel.Padding.Width + GetOptionPanelTotalWidth();
  }
}


int MainFrame::GetMaxHeight() {
  if (rotated_) {
    return iconPanel_->GetMaxHeight() + Styles::MainPanel.Padding.Width + GetOptionPanelTotalWidth();
  } else {
    return iconPanel_->GetMaxHeight() + Styles::MainPanel.Padding.Height;
  }
}


void MainFrame::OnEraseBackground(wxEraseEvent &evt) {

}


void MainFrame::OnPaint(wxPaintEvent& evt) {
  wxPaintDC dc(this);

  if (needLayoutUpdate_) UpdateLayout();
  if (needMaskUpdate_) UpdateMask();
}


void MainFrame::OnMove(wxMoveEvent& evt) {

}


void MainFrame::OnSize(wxSizeEvent& evt) {
  InvalidateLayout();
  InvalidateMask();
}


void MainFrame::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Any MSW application that uses wxWindow::CaptureMouse() must implement an 
  // wxEVT_MOUSE_CAPTURE_LOST event handler as of wxWidgets 2.8.0.
  wxWindow* w = static_cast<wxWindow*>(evt.GetEventObject());
  if (w->HasCapture()) {
    w->ReleaseMouse();
    // Stupid mouse capture hack to make wxWidgets happy
    w->Disconnect(wxID_ANY, wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(MainFrame::OnMouseCaptureLost), NULL, this);
  }
}


void MainFrame::OnMouseDown(wxMouseEvent& evt) {
  wxWindow* w = static_cast<wxWindow*>(evt.GetEventObject());

  w->CaptureMouse();

  if (w->HasCapture()) {
    // Stupid mouse capture hack to make wxWidgets happy
    w->Connect(wxID_ANY, wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(MainFrame::OnMouseCaptureLost), NULL, this);
  }

  windowDragData_.Resizing = false;
  windowDragData_.DraggingStarted = true;
  windowDragData_.InitMousePos = ClientToScreen(evt.GetPosition());
  windowDragData_.InitWindowPos = GetPosition();
}


void MainFrame::OnMouseUp(wxMouseEvent& evt) {
  wxWindow* w = static_cast<wxWindow*>(evt.GetEventObject());
  if (w->HasCapture()) w->ReleaseMouse();
  windowDragData_.DraggingStarted = false;
}


void MainFrame::OnMouseMove(wxMouseEvent& evt) {
  if (windowDragData_.DraggingStarted && evt.Dragging() && evt.LeftIsDown() && !windowDragData_.Resizing) {
    wxPoint mousePos = ClientToScreen(evt.GetPosition());
    wxPoint mouseOffset = mousePos - windowDragData_.InitMousePos;

    int newX = mouseOffset.x + windowDragData_.InitWindowPos.x;
    int newY = mouseOffset.y + windowDragData_.InitWindowPos.y;

    // Do snapping
    int newRight = newX + GetRect().GetWidth();
    int newBottom = newY + GetRect().GetHeight();
    int snappingGap = 10;

    wxDisplay display(GetDisplayIndex());
    wxRect r = display.GetClientArea();

    if (abs(newX - r.GetLeft()) < snappingGap) newX = r.GetLeft();
    if (abs(newRight - r.GetRight()) < snappingGap) newX = r.GetRight() - GetRect().GetWidth();
    if (abs(newY - r.GetTop()) < snappingGap) newY = r.GetTop();
    if (abs(newBottom - r.GetBottom()) < snappingGap) newY = r.GetBottom() - GetRect().GetHeight();    

    Move(newX, newY);
  }
}


void MainFrame::OnResizerMouseDown(wxMouseEvent& evt) {
  wxWindow* w = static_cast<wxWindow*>(evt.GetEventObject());
  w->CaptureMouse();

  windowDragData_.Resizing = true;
  windowDragData_.DraggingStarted = true;
  windowDragData_.InitMousePos = w->ClientToScreen(evt.GetPosition());

  windowDragData_.InitWindowSize.SetWidth(GetSize().GetWidth());
  windowDragData_.InitWindowSize.SetHeight(GetSize().GetHeight());

  windowDragData_.InitWindowPos.x = wxGetApp().GetMainFrame()->GetRect().GetX();
  windowDragData_.InitWindowPos.y = wxGetApp().GetMainFrame()->GetRect().GetY();
}


void MainFrame::OnResizerMouseUp(wxMouseEvent& evt) {
  wxWindow* w = static_cast<wxWindow*>(evt.GetEventObject());
  if (w->HasCapture()) w->ReleaseMouse();
  windowDragData_.DraggingStarted = false;
}


void MainFrame::OnResizerMouseMove(wxMouseEvent& evt) {
  if (windowDragData_.DraggingStarted && evt.Dragging() && evt.LeftIsDown() && windowDragData_.Resizing) {
    wxWindow* w = static_cast<wxWindow*>(evt.GetEventObject());
    
    wxPoint mousePos = w->ClientToScreen(evt.GetPosition());
    wxPoint mouseOffset = mousePos - windowDragData_.InitMousePos;

    int newHeight = windowDragData_.InitWindowSize.GetHeight() + mouseOffset.y;

    if (rotated_) newHeight = windowDragData_.InitWindowSize.GetHeight() - mouseOffset.y;

    if (newHeight < GetMinHeight()) {
      newHeight = GetMinHeight();
    } else if (newHeight > GetMaxHeight()) {
      newHeight = GetMaxHeight();
    }

    int newWidth = windowDragData_.InitWindowSize.GetWidth() + mouseOffset.x;
    if (newWidth < GetMinWidth()) {
      newWidth = GetMinWidth();
    }
    
    // Note: There are no restrictions on maximum width

    if (newWidth == GetSize().GetWidth() && newHeight == GetSize().GetHeight()) return;

    if (rotated_) {
      int newY = windowDragData_.InitWindowPos.y + mouseOffset.y;
      int newX = windowDragData_.InitWindowPos.x;

      SetSize(newX, newY, newWidth, newHeight);
    } else {
      SetSize(newWidth, newHeight);
    }

    int previousPanelWidth = optionPanel_->GetRequiredWidth();

    Update();
    iconPanel_->Update();

    if (optionPanelOpen_) {
      if (previousPanelWidth != optionPanel_->GetRequiredWidth()) {
        optionPanelOpenWidth_ = optionPanel_->GetRequiredWidth();
        InvalidateLayout();
        InvalidateMask();
        Update();
      }
    }

  }
}


void MainFrame::UpdateTransparency() {
  wxTopLevelWindow::SetTransparent(wxGetApp().GetUser()->GetSettings()->GetInt(_T("WindowTransparency")));
}


void MainFrame::InvalidateLayout() {
  needLayoutUpdate_ = true;
  Refresh();
}


void MainFrame::InvalidateMask() {
  needMaskUpdate_ = true;
  Refresh();
}


/**
 * Ensures that any modal dialog is properly closed using
 * EndModal() so that any object waiting for it
 * receives an answer.
 * @param wxWindow* A window to clean up
 */
void MainFrame::RecurseCleanUp(wxWindow* window) {
  wxWindowList& children = window->GetChildren();
  for (int i = children.size() - 1; i >= 0; i--) {
    wxWindow* child = children[i];

    wxString n = child->GetName();
    
    wxDialog* childAsDialog = wxDynamicCast(child, wxDialog);
    if (childAsDialog) {
      if (childAsDialog->IsModal()) {
        childAsDialog->Close();
      } else {
        childAsDialog->Close();
      }
    }

    RecurseCleanUp(child);
  }
}


bool MainFrame::CheckForModalWindow(wxWindow* window) {
  wxWindowList& children = window->GetChildren();
  for (int i = children.size() - 1; i >= 0; i--) {
    wxWindow* child = children[i];

    wxString n = child->GetName();
    
    wxDialog* childAsDialog = wxDynamicCast(child, wxDialog);
    if (childAsDialog) {
      if (childAsDialog->IsModal()) {
        childAsDialog->RequestUserAttention();
        return true;
      }
    }

    bool has = CheckForModalWindow(child);
    if (has) return true;
  }

  return false;
}


bool MainFrame::IsClosing() {
  return isClosing_;
}


void MainFrame::OnClose(wxCloseEvent& evt) {
  if (!initialized_) {    
    ILOG(_T("'Close' action vetoed because the frame is not yet initialized."));
    evt.Veto();
    return;
  }

  if (CheckForModalWindow(this)) {
    ILOG(_T("'Close' action vetoed because a modal window is opened."));
    evt.Veto();
    return;
  }

  if (!IsClosing()) {
    // Start the closing process
    ILOG(_T("Starting close operation (Step 0)"));

    Disable();
    isClosing_ = true;
    closeStep_ = 0;
    while (!DoCloseStep());
  }
}


void MainFrame::OnImageButtonClick(wxCommandEvent& evt) {
  wxWindow* w = static_cast<wxWindow*>(evt.GetEventObject());

  switch(w->GetId()) {
   
    case ID_BUTTON_Arrow:
      
      ToggleOptionPanel();
      break;

    case ID_BUTTON_MainFrame_CloseButton: {

      UserSettings* settings = wxGetApp().GetUser()->GetSettings();
      
      if (settings->GetBool(_T("ShowMinimizeMessage")) && settings->GetBool(_T("MinimizeOnClose"))) {
        int answer = MessageBoxes::ShowInformation(wxString::Format(_("%s is now going to be minimized to the System Tray.\n\nNote that you may change this behavior in the '%s' windows ('%s' tab)"), APPLICATION_NAME, _("Configuration"), _("Operations")), wxOK, _("Don't show this message again"), false);
        if (answer != wxID_OK) return;

        wxGetApp().GetUser()->GetSettings()->SetBool(_T("ShowMinimizeMessage"), !MessageBoxes::GetCheckBoxState());
        wxGetApp().GetUser()->ScheduleSave();
      }      

      if (settings->GetBool(_T("MinimizeOnClose"))) {
        if (settings->GetBool(_T("TrayIcon"))) {
          Hide();
        } else {
          Iconize();
        }
      } else {
        Close();
      }
    } break;

    case ID_BUTTON_MainFrame_EjectButton: {

      wxGetApp().GetUtilities().EjectDriveAndExit();
    } break;

    default:

      evt.Skip();
      break;

  } 
}


void MainFrame::OpenOptionPanel(bool open) {
  if (open == optionPanelOpen_) return;

  optionPanelOpen_ = open;

  if (rotated_) {
    openCloseAnimationWindowRight_ = GetRect().GetBottom();
  } else {
    openCloseAnimationWindowRight_ = GetRect().GetRight();
  }

  if (optionPanelOpen_) {
    optionPanel_->UpdateLayout();
    optionPanelOpenWidth_ = optionPanel_->GetRequiredWidth();
  } else {
    optionPanelOpenWidth_ = 0;
  }

  int newWindowWidth;
  
  if (rotated_) {
    newWindowWidth = arrowButton_->GetSize().GetHeight() + optionPanelOpenWidth_ + backgroundPanel_->GetSize().GetHeight();
  } else {
    newWindowWidth = arrowButton_->GetSize().GetWidth() + optionPanelOpenWidth_ + backgroundPanel_->GetSize().GetWidth();
  }

  if (!rotated_) {
    if (IsLeftOfDisplay()) {
      SetSize(
        newWindowWidth, 
        GetSize().GetHeight());
    } else {
      SetSize(
        openCloseAnimationWindowRight_ - newWindowWidth + 1,
        GetRect().GetTop(),
        newWindowWidth, 
        GetSize().GetHeight());    
    }  
  } else {
    if (IsTopOfDisplay()) {
      SetSize(
        GetSize().GetWidth(),
        newWindowWidth);
    } else {
      SetSize(
        GetRect().GetLeft(),
        openCloseAnimationWindowRight_ - newWindowWidth + 1,
        GetSize().GetWidth(), 
        newWindowWidth);    
    } 
  }

  if (optionPanelOpen_) {
    arrowButton_->SetIcon(arrowButtonOpenIcon_, false);
  } else {
    arrowButton_->SetIcon(arrowButtonCloseIcon_, false);
  }

  InvalidateLayout();
  InvalidateMask();
  Update();
}


void MainFrame::CloseOptionPanel() {
  OpenOptionPanel(false);
}


void MainFrame::ToggleOptionPanel() {
  OpenOptionPanel(!optionPanelOpen_);
}


MainFrame::~MainFrame() {
  // Use MainFrame::OnClose() to free resources
}