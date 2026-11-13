#include "gpu_capture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    char root[]="/tmp/nps64-capture-XXXXXX";
    assert(mkdtemp(root));
    assert(nps64_gpu_capture_prepare(root));
    assert(nps64_gpu_capture_prepare(root));
    char path[1024];snprintf(path,sizeof(path),"%s/gpu-probe/flight",root);assert(!rmdir(path));
    FILE *f=fopen(path,"wb");assert(f);fclose(f);
    assert(!nps64_gpu_capture_prepare(root)); /* A file cannot stand in for a directory. */
    assert(!unlink(path));
    snprintf(path,sizeof(path),"%s/gpu-probe",root);assert(!rmdir(path));assert(!rmdir(root));
    char oversized[1100];memset(oversized,'x',sizeof(oversized)-1);oversized[sizeof(oversized)-1]=0;
    assert(!nps64_gpu_capture_prepare(oversized));
    puts("PASS GPU capture: writable parents, existing directories, file collision and bounded paths");
}
