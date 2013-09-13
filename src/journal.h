#ifndef ASKUE_JOURNAL_H_
#define ASKUE_JOURNAL_H_

#include "config.h"

/* Точка первоначальной настройки базы */
int askue_journal_init ( const journal_cfg_t *JCfg, FILE *Log, int Verbose );

/* Сжать журнал */
int askue_journal_stifle ( const journal_cfg_t *JCfg, FILE *Log, int Verbose );

#endif /* ASKUE_JOURNAL_H_ */
