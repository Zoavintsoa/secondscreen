#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// 0 stopped, 1 starting, 2 ready/waiting for a client, 3 client connected,
// 4 screen-capture permission missing, 5 encoder startup failed,
// 6 display capture startup failed, 7 local video server startup failed.
int SecondScreenGetTestStreamStatus(void);
void SecondScreenStartTestStream(void);
void SecondScreenStopTestStream(void);

#ifdef __cplusplus
}
#endif
