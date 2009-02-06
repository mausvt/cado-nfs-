#ifndef ASYNC_H_
#define ASYNC_H_

#include "parallelizing_info.h"

/* How long do we typically accept to wait for asynchronous output (=
 * signals) to be checked ? */
#define PREFERRED_ASYNC_LAG     15.0

struct timing_data {
    int go_mark;
    int last_print;
    int next_print;
    int next_async_check;
    int async_check_period;
    struct {
        double job;
        double thread;
        double wct;
    } go[1], current[1];
};

#ifdef __cplusplus
extern "C" {
#endif

void timing_init(struct timing_data * t, int iter);
void timing_clear(struct timing_data * t);
void timing_check(parallelizing_info pi, struct timing_data * t, int iter);
void block_control_signals();
void catch_control_signals();

#ifdef __cplusplus
}
#endif

#endif	/* ASYNC_H_ */
