/*
  Copyright (C) 2008-2010 Laurent Cozic. All right reserved.
  Use of this source code is governed by a GNU/GPL license that can be
  found in the LICENSE file.
*/

#include <stable.h>

#include <GraphicsWindow.h>

#ifndef Appetizer_GraphicsScene_H
#define Appetizer_GraphicsScene_H

namespace appetizer {


class GraphicsScene : public QGraphicsScene {

public:

  GraphicsScene(GraphicsWindow* parentWindow);
  ~GraphicsScene();
  inline GraphicsWindow* parentWindow() { return parentWindow_; }
  QList<QGraphicsItem*> topLevelItems();
  void paintAll(QPainter* painter);
  
private:
	
  GraphicsWindow* parentWindow_;
  void paintAll_(QPainter* painter, QGraphicsItem* item, int x, int y);

};

}
#endif // Appetizer_GraphicsScene_H
