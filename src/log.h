#ifndef ASKUE_LOG_H_
#define ASKUE_LOG_H_

#include <stdio.h>

#include "config.h"

// открыть лог
int askue_log_open ( FILE **Ptr, const askue_cfg_t *Cfg );
// закрыть лог
void askue_log_close ( FILE **Log );

// обрезать лог
int askue_log_stifle ( FILE **Log, const askue_cfg_t *Cfg );

#endif /* ASKUE_LOG_H_ */
