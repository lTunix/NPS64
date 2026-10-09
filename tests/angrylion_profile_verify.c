#define main renderer_fixture_main
#include "../vendor/parallel-n64/tools/angrylion_verify.c"
#undef main
extern void nps64_core_game_profile(unsigned) __attribute__((weak));
int main(int argc,char **argv) {
    const char *profile=getenv("NPS64_TEST_GAME_PROFILE");
    if(nps64_core_game_profile)nps64_core_game_profile(profile?strtoul(profile,NULL,10):0);
    return renderer_fixture_main(argc,argv);
}
