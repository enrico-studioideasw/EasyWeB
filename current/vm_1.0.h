#ifndef EWB_VM_1_0_H
#define EWB_VM_1_0_H

#ifdef __cplusplus
extern "C" {
#endif

int ewb_run_text(const char *program, int entrypoint, int stackpos, const char *encoded_stack);

#ifdef __cplusplus
}
#endif

#endif
