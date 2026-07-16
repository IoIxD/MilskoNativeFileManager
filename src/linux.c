
#define _GNU_SOURCE
#include <dlfcn.h>

#define _MILSKO
#include <MNFM.h>
#include <dbus/dbus.h>

static struct dbus_funcs_t {
  void *dbus_lib;
  dbus_bool_t (*dbus_threads_init_default)(void);
  void (*dbus_error_init)(DBusError *error);
  DBusConnection *(*dbus_bus_get_private)(DBusBusType type, DBusError *error);
  void (*dbus_error_free)(DBusError *error);
  dbus_bool_t (*dbus_connection_read_write_dispatch)(DBusConnection *connection,
                                                     int timeout_milliseconds);
  DBusMessage *(*dbus_message_new_method_call)(const char *bus_name,
                                               const char *path,
                                               const char *iface,
                                               const char *method);
  void (*dbus_message_iter_init_append)(DBusMessage *message,
                                        DBusMessageIter *iter);
  dbus_bool_t (*dbus_message_iter_append_basic)(DBusMessageIter *iter, int type,
                                                const void *value);
  dbus_bool_t (*dbus_message_iter_open_container)(
      DBusMessageIter *iter, int type, const char *contained_signature,
      DBusMessageIter *sub);
  dbus_bool_t (*dbus_message_iter_close_container)(DBusMessageIter *iter,
                                                   DBusMessageIter *sub);
  dbus_bool_t (*dbus_connection_send_with_reply)(
      DBusConnection *connection, DBusMessage *message,
      DBusPendingCall **pending_return, int timeout_milliseconds);
  void (*dbus_message_unref)(DBusMessage *message);
  void (*dbus_connection_flush)(DBusConnection *connection);
  void (*dbus_pending_call_block)(DBusPendingCall *pending);
  DBusMessage *(*dbus_pending_call_steal_reply)(DBusPendingCall *pending);
  void (*dbus_pending_call_unref)(DBusPendingCall *pending);
  dbus_bool_t (*dbus_message_iter_init)(DBusMessage *message,
                                        DBusMessageIter *iter);
  void (*dbus_message_iter_get_basic)(DBusMessageIter *iter, void *value);
  dbus_bool_t (*dbus_connection_try_register_object_path)(
      DBusConnection *connection, const char *path,
      const DBusObjectPathVTable *vtable, void *user_data, DBusError *error);
  dbus_bool_t (*dbus_message_iter_next)(DBusMessageIter *iter);
  void (*dbus_message_iter_recurse)(DBusMessageIter *iter,
                                    DBusMessageIter *sub);
  int (*dbus_message_iter_get_arg_type)(DBusMessageIter *iter);
  dbus_bool_t (*dbus_message_iter_has_next)(DBusMessageIter *iter);
} dbus;

static MwBool dbus_valid = MwTRUE;

typedef struct internal_t {
  MwBool valid;

  DBusError err;
  DBusConnection *conn;
  DBusMessage *message;
  DBusMessage *reply;
  int status;
  MwBool opened;

  char title[255];
  MNFMCreationType creation_type;

} internal;

static const char *dbgstr_dbus_error(const DBusError *error);
static DBusHandlerResult
file_chooser_response_handler(DBusConnection *connection, DBusMessage *message,
                              void *data);

static const DBusObjectPathVTable file_chooser_response_vtable = {
    .message_function = file_chooser_response_handler,
};

static int wcreate(MwWidget handle) {
  internal *o = handle->internal = malloc(sizeof(internal));
  memset(o, 0, sizeof(internal));

  dbus.dbus_threads_init_default();
  dbus.dbus_error_init(&o->err);

  o->conn = dbus.dbus_bus_get_private(DBUS_BUS_SESSION, &o->err);
  if (!o->conn) {
    printf("Failed to get system dbus connection: %s\n",
           dbgstr_dbus_error(&o->err));
    dbus.dbus_error_free(&o->err);
    o->valid = MwFALSE;
    return 1;
  }

  o->valid = MwTRUE;

  return 0;
}

static void destroy(MwWidget handle) { return; }

static int _MNFMOpen(MwWidget handle, const char *title,
                     MNFMCreationType creationType) {
  DBusMessage *msg;
  DBusMessageIter margs;
  DBusMessageIter options;
  DBusPendingCall *pending;
  const char *handle_str;
  internal *o = handle->internal;
  const char *parent_window = "";

  msg = dbus.dbus_message_new_method_call(
      "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.FileChooser", "OpenFile");

  if (msg == NULL) {
    printf("no memory\n");
    return 1;
  }

  // msg != NULL
  dbus.dbus_message_iter_init_append(msg, &margs);
  dbus.dbus_message_iter_append_basic(&margs, DBUS_TYPE_STRING,
                                      &parent_window); // parent_window
  dbus.dbus_message_iter_append_basic(&margs, DBUS_TYPE_STRING,
                                      &title); // title
  dbus.dbus_message_iter_open_container(&margs, 'a', "{sv}", &options);

  if (creationType == MNFMDIRECTORY) {
    DBusMessageIter entry, value;
    const char *key = "directory";
    dbus_bool_t dir = TRUE;

    dbus.dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, NULL,
                                          &entry);
    dbus.dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus.dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b",
                                          &value);
    dbus.dbus_message_iter_append_basic(&value, DBUS_TYPE_BOOLEAN, &dir);
    dbus.dbus_message_iter_close_container(&entry, &value);
    dbus.dbus_message_iter_close_container(&options, &entry);
  }

  dbus.dbus_message_iter_close_container(&margs, &options);

  if (!dbus.dbus_connection_send_with_reply(o->conn, msg, &pending, -1)) {
    printf("Did not get pending call response for file dialog request: %s\n",
           dbgstr_dbus_error(&o->err));
    dbus.dbus_error_free(&o->err);
    dbus.dbus_message_unref(msg);
    return 1;
  }

  if (pending == NULL) {
    printf("Did not get pending call response for file dialog request: %s\n",
           dbgstr_dbus_error(&o->err));
    dbus.dbus_error_free(&o->err);
    dbus.dbus_message_unref(msg);
    return 1;
  }

  dbus.dbus_connection_flush(o->conn);
  dbus.dbus_message_unref(msg);

  // Wait for response
  dbus.dbus_pending_call_block(pending);
  msg = dbus.dbus_pending_call_steal_reply(pending);
  if (msg == NULL) {
    printf("Did not get pending call response for file dialog request: %s\n",
           dbgstr_dbus_error(&o->err));
    dbus.dbus_error_free(&o->err);
    dbus.dbus_pending_call_unref(pending);
    return 1;
  }
  dbus.dbus_pending_call_unref(pending);

  // Read handle
  dbus.dbus_message_iter_init(msg, &margs);
  dbus.dbus_message_iter_get_basic(&margs, &handle_str);

  dbus.dbus_message_unref(msg);

  o->status = -1;

  dbus.dbus_connection_try_register_object_path(
      o->conn, handle_str, &file_chooser_response_vtable, handle, &o->err);

  return 0;
}

static void tick(MwWidget handle) {
  internal *o = handle->internal;
  if (!o->opened) {
    _MNFMOpen(handle, MwGetText(handle, MwNtitle), o->creation_type);
    o->opened = MwTRUE;
  }
  if (o->status != -1) {
    return;
  }
  dbus.dbus_connection_read_write_dispatch(o->conn, 50);
}

static const char *dbgstr_dbus_error(const DBusError *error) {
  char *err = malloc(sizeof(char) * 255);
  snprintf(err, 255, "{%s: %s}", (error->name), (error->message));
  return err;
}

static DBusHandlerResult
file_chooser_response_handler(DBusConnection *connection, DBusMessage *message,
                              void *data) {
  MwWidget handle = (MwWidget)data;
  internal *internal = handle->internal;

  DBusMessageIter margs, results, entry, variant, uris;
  const char *key, *uri;

  dbus.dbus_message_iter_init(message, &margs);
  dbus.dbus_message_iter_get_basic(&margs, &internal->status);

  // Recurse into results
  dbus.dbus_message_iter_next(&margs);
  dbus.dbus_message_iter_recurse(&margs, &results);
  for (;;) {
    dbus.dbus_message_iter_recurse(&results, &entry);

    dbus.dbus_message_iter_get_basic(&entry, &key);
    dbus.dbus_message_iter_next(&entry);
    dbus.dbus_message_iter_recurse(&entry, &variant);

    if (strcmp(key, "uris") == 0) {
      dbus.dbus_message_iter_recurse(&variant, &uris);
      if (dbus.dbus_message_iter_get_arg_type(&uris) != 0) {
        const char *uri_chopped;
        dbus.dbus_message_iter_get_basic(&uris, &uri);
        if (strstr(uri, "file://")) {
          uri_chopped = uri + 7;
        } else {
          uri_chopped = uri;
        }
        if (internal->creation_type == MNFMDIRECTORY) {
          MwDispatchUserHandler(handle, MwNdirectoryChosenHandler,
                                (void *)uri_chopped);
        } else {
          MwDispatchUserHandler(handle, MwNfileChosenHandler,
                                (void *)uri_chopped);
        }
      }

      break;
    }

    if (!dbus.dbus_message_iter_has_next(&results))
      break;
    dbus.dbus_message_iter_next(&results);
  }

  return DBUS_HANDLER_RESULT_HANDLED;
}

void _MNFMSetVars(MwWidget handle, MNFMCreationType creationType) {
  ((internal *)handle->internal)->creation_type = creationType;
};

MwBool _MNFMLibraryValid() { return dbus_valid; }

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
    tick,    /* tick */
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

void MNFMLibraryInit(void) {
  dbus.dbus_lib = dlopen("libdbus-1.so", RTLD_GLOBAL | RTLD_NOW);
  if (!dbus.dbus_lib) {
    printf("Cannot find libdbus-1.so, falling back to Milsko file picker.\n");
    dbus_valid = MwFALSE;
    return;
  }

#define DBUS_FUNC(x)                                                           \
  dbus.x = dlsym(dbus.dbus_lib, #x);                                           \
  if (!dbus.x) {                                                               \
    printf("Couldn't load " #x ", falling back to Milsko file picker.\n");     \
    return;                                                                    \
  }

  DBUS_FUNC(dbus_threads_init_default);
  DBUS_FUNC(dbus_error_init);
  DBUS_FUNC(dbus_bus_get_private);
  DBUS_FUNC(dbus_connection_read_write_dispatch);
  DBUS_FUNC(dbus_message_new_method_call);
  DBUS_FUNC(dbus_message_iter_init_append);
  DBUS_FUNC(dbus_message_iter_append_basic);
  DBUS_FUNC(dbus_message_iter_open_container);
  DBUS_FUNC(dbus_message_iter_close_container);
  DBUS_FUNC(dbus_connection_send_with_reply);
  DBUS_FUNC(dbus_message_unref);
  DBUS_FUNC(dbus_connection_flush);
  DBUS_FUNC(dbus_pending_call_block);
  DBUS_FUNC(dbus_pending_call_steal_reply);
  DBUS_FUNC(dbus_pending_call_unref);
  DBUS_FUNC(dbus_message_iter_init);
  DBUS_FUNC(dbus_message_iter_get_basic);
  DBUS_FUNC(dbus_connection_try_register_object_path);
  DBUS_FUNC(dbus_message_iter_next);
  DBUS_FUNC(dbus_message_iter_recurse);
  DBUS_FUNC(dbus_message_iter_get_arg_type);
  DBUS_FUNC(dbus_message_iter_has_next);
};
