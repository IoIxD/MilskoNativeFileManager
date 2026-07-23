
#include <ShObjIdl.h>
#include <combaseapi.h>
#include <windows.h>

#define _MILSKO
#include <MNFM.h>

typedef struct internal_t {
  MwBool valid;
  MNFMCreationType creation_type;

  union {
    IFileOpenDialog *pfd;
    IFileSaveDialog *psd;
  };
  HANDLE thread;

  MwWidget handle;
  WCHAR wtitle[512];

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

  /* make sure we can create an instance. */
  hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IFileOpenDialog, (void **)&o->pfd);
  if (!SUCCEEDED(hr)) {
    printf("CoCreateInstance failed. Falling back to Milsko file picker.");
    o->valid = MwFALSE;
    return 0;
  }
  /* if we can then remove it because we're gonna create it later */
  o->pfd->lpVtbl->Release(o->pfd);
  DeleteObject(o->pfd);

  o->handle = handle;

  o->pfd = NULL;

  o->valid = MwTRUE;


  return 0;
}

static void destroy(MwWidget handle) {
  internal *o = handle->internal;

  if (o->creation_type == MNFMSAVE) {
    if (o->psd) {
      o->psd->lpVtbl->Close(o->psd, 0);
      o->psd->lpVtbl->Release(o->psd);
      DeleteObject(o->psd);
    }
  } else {
    if (o->pfd) {
      o->pfd->lpVtbl->Close(o->pfd, 0);
      o->pfd->lpVtbl->Release(o->pfd);
      DeleteObject(o->pfd);
    }
  }
  if (o->thread) {
    TerminateThread(o->thread, 0);
  }

  return;
}

static DWORD WINAPI folder_show(LPVOID lpParam) {
  internal *o = lpParam;
  IShellItem *arr = NULL;
  HRESULT hr;
  FILEOPENDIALOGOPTIONS opts;

  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  if (o->creation_type == MNFMSAVE) {
    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IFileSaveDialog, (void **)&o->psd);
  } else {
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IFileOpenDialog, (void **)&o->pfd);
  }

  if(!SUCCEEDED(hr)) {
      printf("CoCreateInstance failed! %08lX\n",hr);
      return 0;
  }

  if (o->creation_type == MNFMDIRECTORY) {
    o->pfd->lpVtbl->GetOptions(o->pfd, &opts);
    opts |= FOS_PICKFOLDERS;
    o->pfd->lpVtbl->SetOptions(o->pfd, opts);
  }

  if (o->creation_type == MNFMSAVE) {
    o->psd->lpVtbl->SetTitle(o->psd, o->wtitle);
    o->psd->lpVtbl->Show(o->psd, NULL);
  } else {
    o->pfd->lpVtbl->SetTitle(o->pfd, o->wtitle);
    o->pfd->lpVtbl->Show(o->pfd, NULL);
  }

  while (!arr) {
    if (o->creation_type == MNFMSAVE) {
      o->psd->lpVtbl->GetResult(o->psd, &arr);
    } else {
      o->pfd->lpVtbl->GetResult(o->pfd, &arr);
    }

    if (arr) {
      WCHAR *wname = NULL;
      char name[255];
      int hr = arr->lpVtbl->GetDisplayName(arr, SIGDN_FILESYSPATH, &wname);

      if (hr != S_OK) {
        printf("GetDisplayName Error %08X\n", hr);
      }
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

  if (o->creation_type == MNFMSAVE) {
    o->psd->lpVtbl->Release(o->psd);
    DeleteObject(o->psd);
    o->psd = NULL;
  } else {
    o->pfd->lpVtbl->Release(o->pfd);
    DeleteObject(o->pfd);
    o->pfd = NULL;
  }

  return 0;
}

void _MNFMSetVars(MwWidget handle, MNFMCreationType creationType) {
  internal *o = handle->internal;
  const char *title = MwGetText(handle, MwNtitle);
  o->creation_type = creationType;

  MultiByteToWideChar(CP_UTF8, 0, title, -1, o->wtitle, sizeof(o->wtitle));

  CreateThread(NULL,        // default security attributes
               0,           // use default stack size
               folder_show, // thread function name
               o,           // argument to thread function
               0,           // use default creation flags
               o->thread);  // returns the thread identifier
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

void MNFMLibraryInit(void) { CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); };
