#ifndef _BOZ_APP_LIST
#define _BOZ_APP_LIST

const struct boz_app *
boz_app_list_get(void);

int
boz_app_list_get_length(void);

int
boz_app_lookup_id(int id, struct boz_app *dest);

#endif
