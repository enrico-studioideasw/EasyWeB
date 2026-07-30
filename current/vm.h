#ifndef EWB_VM_H
#define EWB_VM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int ewb_run_text(const char *program, const char *program_url, int entrypoint, int stackpos, const char *encoded_stack);
int ewb_run_buffer(const char *program, size_t len, const char *program_url, int entrypoint, int stackpos, const char *encoded_stack);
void ewb_form_clear(void);
void ewb_form_add_field(const char *name, const char *value, size_t size);
void ewb_form_add_file(const char *name, const char *filename,
                       const char *content_type, const char *path, size_t size);

#ifdef __cplusplus
}
#endif

#endif
