#include "NineSlicesPanel.h"
#include "wx/dcbuffer.h"
#include "BitmapControlInterface.h"



INITIALIZE_BITMAP_CONTROL_INTERFACE(NineSlicesPanel)


BEGIN_EVENT_TABLE(NineSlicesPanel, wxPanel)
  EVT_PAINT(NineSlicesPanel::OnPaint)
  EVT_ERASE_BACKGROUND(NineSlicesPanel::OnEraseBackground)
END_EVENT_TABLE()



NineSlicesPanel::NineSlicesPanel(wxWindow *owner, int id, wxPoint point, wxSize size):
wxPanel(owner, id, point, size, 0 | wxFRAME_SHAPED | wxNO_BORDER | wxTRANSPARENT_WINDOW) {
  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  pNineSlicesPainter = NULL;
}


void NineSlicesPanel::LoadImage(const wxString& filePath) {
  if (pFilePath == filePath) return;

  wxDELETE(pNineSlicesPainter);
  
  pFilePath = filePath;
  InvalidateControlBitmap();
}


void NineSlicesPanel::OnEraseBackground(wxEraseEvent &evt) {

}


void NineSlicesPanel::UpdateControlBitmap() {
  if ((pNineSlicesPainter == NULL) && (pFilePath != "")) {
    pNineSlicesPainter = new NineSlicesPainter();
    pNineSlicesPainter->LoadImage(pFilePath);
  }

  if (pNineSlicesPainter != NULL) {
    pControlBitmap.~wxBitmap();
    pControlBitmap = wxBitmap(GetClientRect().GetWidth(), GetClientRect().GetHeight());
    pSourceDC.SelectObjectAsSource(pControlBitmap);	
    pNineSlicesPainter->Draw(&pSourceDC, 0, 0, GetClientRect().GetWidth(), GetClientRect().GetHeight());
  } else {
    pSourceDC.SelectObjectAsSource(wxNullBitmap);
  }
}


void NineSlicesPanel::OnPaint(wxPaintEvent& evt) {
  wxBufferedPaintDC dc(this);  

  UPDATE_CONTROL_BITMAP_IF_NEEDED()

  dc.Blit(0, 0, GetClientRect().GetWidth(), GetClientRect().GetHeight(), &pSourceDC, 0, 0);
}