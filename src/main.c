#include "app.h" // for TpvApp, tpv_init, tpv_free

int main(int argc, char** argv) {
    TpvApp app = tpv_init(argc, argv);
    tpv_run(&app);
    tpv_free(&app);
}
