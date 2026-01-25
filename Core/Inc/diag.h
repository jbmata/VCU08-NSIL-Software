#ifndef DIAG_H
#define DIAG_H

#include <stdint.h>
#include "cmsis_os2.h"

void Diag_Report(osMessageQueueId_t rxQ, osMessageQueueId_t txQ);

/* Hook: implement in your project (UART/log). Default weak no-op. */
void Diag_Log(const char *s);

#endif /* DIAG_H */
