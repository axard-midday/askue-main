#ifndef ASKUE_ECODE_H_
#define ASKUE_ECODE_H_


// | 4 бита доп. информации | 4 бита осн. информации |

#define SET_MINFO( Var, Info ) ({ Var |= Info; })
#define GET_MINFO( Var, Info ) ({ Var & 0x0f; })

#define SET_AINFO( Var, Info ) ({ Var |= ( Info << 4 ); })
#define GET_AINFO( Var, Info ) ({ Var & 0xf0; })

// --------------------------------------------------------------------
// Коды основной информации

// результат выполнения - ошибка
#define ASKUE_ERROR -1

// результат выполнения - запрос на переконфигурацию
#define ASKUE_RECONF 2

// результат выполнения - успех
#define ASKUE_SUCCESS 0

// результат выполнения - необходимо штатное завершение
#define ASKUE_EXIT 1

// --------------------------------------------------------------------
// Коды дополнительной информации

#define ASKUE_CONNECT 1

#endif /* ASKUE_ECODE_H_ */
