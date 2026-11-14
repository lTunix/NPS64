#define main frontend_main
#include "../src/frontend.c"
#undef main
#include <assert.h>
#include <unistd.h>
static unsigned char memory[512];static unsigned writes;static bool fail_write;
size_t retro_get_memory_size(unsigned type){(void)type;return sizeof(memory);}
void *retro_get_memory_data(unsigned type){(void)type;return memory;}
void nps64_log(const char *format,...){(void)format;}
bool __real_file_write_atomic(const char*,const void*,size_t);
bool __wrap_file_write_atomic(const char *path,const void *data,size_t size){writes++;return !fail_write&&__real_file_write_atomic(path,data,size);}
int main(void){
 mkdir(DATA_ROOT,0777);mkdir(DATA_ROOT "/saves",0777);strcpy(current.key,"rom-0000000000000001");char path[PATH_CAP];save_path(path,".srm");unlink(path);
 memset(memory,0x32,sizeof(memory));assert(save_ram()&&sram_wrote&&writes==1);
 assert(save_ram()&&!sram_wrote&&writes==1);
 memory[3]++;assert(save_ram()&&writes==2);unsigned char stored[sizeof(memory)];assert(file_read_exact(path,stored,sizeof(stored))&&!memcmp(memory,stored,sizeof(memory)));
 memory[9]++;fail_write=true;assert(!save_ram()&&writes==3&&!sram_wrote);
 fail_write=false;assert(save_ram()&&writes==4);assert(save_ram()&&writes==4);
 ram_writes_allowed=false;memory[7]++;assert(!save_ram()&&writes==4);ram_writes_allowed=true;
 unlink(path);assert(save_ram()&&writes==5); /* Deleted file must be recreated. */
 assert(save_ram()&&writes==5);
 nps64_save_cache_reset(&sram_cache);assert(save_ram()&&writes==6); /* New session cannot reuse an unverified baseline. */
 nps64_save_cache_remember(&sram_cache,NULL,0);assert(!sram_cache.valid&&!sram_cache.bytes);
 nps64_save_cache_reset(&sram_cache);unlink(path);rmdir(DATA_ROOT "/saves");rmdir(DATA_ROOT);
 puts("PASS SRAM cache: initial/changed writes, unchanged skip, exact disk bytes, failed-write retry, protected RAM, deleted file and session reset");
}
