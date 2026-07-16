
#include <ShObjIdl.h>
#include <combaseapi.h>
#include <windows.h>

#define _MILSKO
#include <MNFM.h>

typedef struct internal_t {
  MwBool valid;
  MNFMCreationType creation_type;

  IFileOpenDialog *pfd;
  DWORD thread;

  MwWidget handle;

} internal;

#define COM_MUST_CORRECT(x)                                                    \
  hr = x;                                                                      \
  if (!SUCCEEDED(hr)) {                                                        \
    printf("FAILED: " #x ". Falling back to Milsko file picker.");             \
    o->valid = MwFALSE;                                                        \
    return 0;                                                                  \
  }

static int wcreate(MwWidget handle) {
  HRESULT hr;
  internal *o = handle->internal = malloc(sizeof(internal));

  memset(o, 0, sizeof(internal));

  COM_MUST_CORRECT(CoCreateInstance(&CLSID_FileOpenDialog, NULL,
                                    CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog,
                                    (void **)&o->pfd))

  o->handle = handle;

  o->valid = MwTRUE;

  return 0;
}

static void destroy(MwWidget handle) {
  internal *o = handle->internal;
  o->pfd->lpVtbl->Close(o->pfd, 0);

  o->pfd->lpVtbl->Release(o->pfd);

  DeleteObject(o->pfd);

  return;
}

static DWORD WINAPI folder_show(LPVOID lpParam) {
  internal *o = lpParam;
  IShellItem *arr = NULL;

  o->pfd->lpVtbl->Show(o->pfd, NULL);

  while (!arr) {
    o->pfd->lpVtbl->GetResult(o->pfd, &arr);

    if (arr) {
      WCHAR *wname = NULL;
      char name[255];
      arr->lpVtbl->GetDisplayName(arr, SIGDN_FILESYSPATH, &wname);
      memset(name, 0, sizeof(name));

      WideCharToMultiByte(CP_UTF8, 0, wname, -1, name, sizeof(name) - 1, NULL,
                          NULL);

      if (o->creation_type == MNFMDIRECTORY) {
        MwDispatchUserHandler(o->handle, MwNdirectoryChosenHandler,
                              (void *)name);
      } else {
        MwDispatchUserHandler(o->handle, MwNfileChosenHandler, (void *)name);
      }
    }
  }

  return 0;
}

void _MNFMSetVars(MwWidget handle, MNFMCreationType creationType) {
  HRESULT hr;
  internal *o = handle->internal;
  WCHAR wtitle[512];
  FILEOPENDIALOGOPTIONS opts;
  const char *title = MwGetText(handle, MwNtitle);
  o->creation_type = creationType;

  MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, sizeof(wtitle));

  if (creationType == MNFMDIRECTORY) {
    o->pfd->lpVtbl->GetOptions(o->pfd, &opts);
    opts |= FOS_PICKFOLDERS;
    o->pfd->lpVtbl->SetOptions(o->pfd, opts);
  }

  o->pfd->lpVtbl->SetTitle(o->pfd, wtitle);

  CreateThread(NULL,        // default security attributes
               0,           // use default stack size
               folder_show, // thread function name
               o,           // argument to thread function
               0,           // use default creation flags
               &o->thread); // returns the thread identifier
};

MwBool _MNFMLibraryValid() { return MwTRUE; }

MwBool _MNFMValid(MwWidget handle) {
  return ((internal *)handle->internal)->valid;
}

MwClassRec MNFMWidgetClassRec = {
    wcreate, /* create */
    destroy, /* destroy */
    NULL,    /* draw */
    NULL,    /* click */
    NULL,    /* parent_resize */
    NULL,    /* prop_change */
    NULL,    /* mouse_move */
    NULL,    /* mouse_up */
    NULL,    /* mouse_down */
    NULL,    /* key */
    NULL,    /* execute */
    NULL,    /* tick */
    NULL,    /* resize */
    NULL,    /* children_update */
    NULL,    /* children_prop_change */
    NULL,    /* clipboard */
    NULL,    /* props_change */
    NULL,    /* reserved */
    NULL,    /* reserved */
    NULL,    /* reserved */
};

MwClass MNFMWidgetClass = &MNFMWidgetClassRec;

void MNFMLibraryInit(void) { CoInitializeEx(NULL, COINIT_MULTITHREADED); };
