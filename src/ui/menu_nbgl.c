#ifdef HAVE_NBGL

#include "os.h"
#include "nbgl_content.h"
#include "nbgl_use_case.h"
#include "menu.h"

//  -----------------------------------------------------------
//  --------------------- SETTINGS MENU -----------------------
//  -----------------------------------------------------------
#define SETTING_INFO_NB 2
static const char* const INFO_TYPES[SETTING_INFO_NB] = {"Version", "Developer"};
static const char* const INFO_CONTENTS[SETTING_INFO_NB] = {APPVERSION, "Blooo"};
static const nbgl_contentInfoList_t infoList = {
    .nbInfos = SETTING_INFO_NB,
    .infoTypes = INFO_TYPES,
    .infoContents = INFO_CONTENTS,
};

//  -----------------------------------------------------------
//  ----------------------- HOME PAGE -------------------------
//  -----------------------------------------------------------
void app_quit(void) {
    // exit app here
    os_sched_exit(-1);
}

void ui_main_menu(void) {
    nbgl_useCaseHomeAndSettings(APPNAME,
                                &C_app_everscale_64px,
                                NULL,
                                INIT_HOME_PAGE,
                                NULL,
                                &infoList,
                                NULL,
                                app_quit);
}

#endif