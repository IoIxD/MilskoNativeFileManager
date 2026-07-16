#include <MNFM.h>
#ifndef _WIN32
#include <unistd.h>
#endif

extern MwClass MNFMWidgetClass;

void _MNFMSetVars(MwWidget handle, MNFMCreationType creationType);
MwBool _MNFMValid(MwWidget handle);
MwBool _MNFMLibraryValid();

MwWidget MNFMOpen(MwWidget handle, const char *title,
                  MNFMCreationType creationType) {
  MwWidget mnfm;
  if (!_MNFMLibraryValid()) {
    goto fallback;
  }

  mnfm = MwVaCreateWidget(MNFMWidgetClass, "", handle, MwDEFAULT, MwDEFAULT,
                          640, 480, MwNtitle, title, NULL);

  if (_MNFMValid(mnfm)) {
    _MNFMSetVars(mnfm, creationType);
    return mnfm;
  } else {
    MwFreeWidget(mnfm);
  fallback:
    return MwFileChooserEx(handle, title,
                           (creationType == MNFMDIRECTORY) ? 1 : 0);
  }
};
