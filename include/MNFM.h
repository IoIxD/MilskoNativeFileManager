#ifndef __MNFM_H__
#define __MNFM_H__

#include <Mw/Milsko.h>

enum _MNFMCreationType {
  MNFMFILE = 0,
  MNFMDIRECTORY,
};
/* Whether to open a file or a directory */
typedef enum _MNFMCreationType MNFMCreationType;

/* Initialize the MNFM Library */
MWDECL void MWAPI MNFMLibraryInit(void);

/* Open the native file chooser, or fallback to Milsko's if we're unable to */
MWDECL MwWidget MWAPI MNFMOpen(MwWidget handle, const char *title,
                               MNFMCreationType create_type);

#endif
