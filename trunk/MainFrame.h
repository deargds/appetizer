#include "wx/wx.h" 
#include "NineSlicesPainter.h"
#include "ImagePanel.h"


class MainFrame: public wxFrame {

  private:

    struct WindowDragDataStruct {
      bool DraggingStarted;
      wxPoint InitWindowPos;
      wxPoint InitMousePos;
    };

    wxBitmap pMaskBitmap;
    NineSlicesPainter pMaskNineSlices;
    NineSlicesPainter pBackgroundNineSlices;    
    WindowDragDataStruct pWindowDragData;
    wxImage* pResizerImage;
    ImagePanel* pResizerPanel;

    void UpdateMask();
    void UpdateLayout();

  public:

    // Constructor
    MainFrame();

    // Event handlers
    void OnPaint(wxPaintEvent& evt);
    void OnWindowCreate(wxWindowCreateEvent& evt);
    void OnMouseDown(wxMouseEvent& evt);
    void OnMouseUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);

  DECLARE_EVENT_TABLE()

};

