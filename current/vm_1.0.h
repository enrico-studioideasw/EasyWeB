#ifndef EWB_VM_1_0_H
#define EWB_VM_1_0_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int ewb_run_text(const char *program, const char *program_url, int entrypoint, int stackpos, const char *encoded_stack);
int ewb_run_buffer(const char *program, size_t len, const char *program_url, int entrypoint, int stackpos, const char *encoded_stack);

#ifdef __cplusplus
}
#endif

#endif
