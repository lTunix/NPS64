#ifndef NPS64_FRAME_CAPTURE_H
#define NPS64_FRAME_CAPTURE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool nps64_frame_capture_arm(const char *root,const char *rom,unsigned width,unsigned height,const char *trigger);
void nps64_frame_capture_before(const uint32_t *pixels,uint64_t core_frame,uint64_t video_frame,unsigned source_width,unsigned source_height,unsigned scale);
void nps64_frame_capture_after(const uint32_t *pixels);
void nps64_frame_capture_finish(void);
void nps64_frame_capture_shutdown(void);
int nps64_frame_capture_result(void);
#endif
