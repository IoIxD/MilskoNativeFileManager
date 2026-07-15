#include <MNFM.h>
#include <Mw/Milsko.h>

MwWidget fileName;

static void file_chosen(MwWidget handle, void *user_data, void *call_data) {
  MwSetText(fileName, MwNtext, (char *)call_data);
};

int main() {
  MwWidget window;
  MwWidget fileChoser;

  MwLibraryInit();
  MNFMLibraryInit(); /* can be called before or after */

  window =
      MwCreateWidget(MwWindowClass, "", NULL, MwDEFAULT, MwDEFAULT, 640, 480);

  fileChoser = MNFMOpen(window, "Open a File", MNFMFILE);

  MwAddUserHandler(fileChoser, MwNfileChosenHandler, file_chosen, NULL);

  fileName = MwVaCreateWidget(MwLabelClass, "", window, 10, 0, 640, 480,
                              MwNalignment, MwALIGNMENT_CENTER, NULL);

  MwLoop(window);

  return 0;
}
