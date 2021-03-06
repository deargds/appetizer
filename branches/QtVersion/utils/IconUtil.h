/*
  Copyright (C) 2008-2010 Laurent Cozic. All right reserved.
  Use of this source code is governed by a GNU/GPL license that can be
  found in the LICENSE file.
*/

#include <stable.h>

#ifndef IconUtil_H
#define IconUtil_H

#include <IconData.h>

namespace appetizer {


// library function is: HRESULT SHGetImageList(int iImageList, REFIID riid, void **ppv)
typedef HRESULT (CALLBACK* SHGetImageListType)(int, const IID&, void*);

class IconUtil {

public:

  static IconData* getFolderItemIcon(const QString& filePath, int iconSize);
  static IconData* getExecutableIcon(const QString& filePath, int iconSize, int iconIndex = 0);

  static QPixmap* iconDataToPixmap(IconData* iconData);

};

}
#endif // IconUtil_H
