#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <libaskue/write_msg.h>
#include <libaskue/macro.h>
#include <libaskue/my.h>
#include <ctype.h>

#include "config.h"
#include "tbuffer.h"
#include "ecode.h"
#include "flag.h"

#define askue_config_fail( file, msg ) write_msg ( file, "Конфигурация", "FAIL", msg );
#define askue_config_error( file, msg ) write_msg ( file, "Конфигурация", "ERROR", msg );
#define askue_config_ok( file, msg ) write_msg ( file, "Конфигурация", "OK", msg );
#define askue_config_info( file, msg ) write_msg ( file, "Конфигурация", "INFO", msg );


/**********************************************************************/

/* это последнее устройство */
int is_last_device ( const device_cfg_t *TheDevice )
{
    return ( TheDevice->Name == NULL ) &&
            ( TheDevice->Type == NULL ) &&
            ( TheDevice->Timeout == 0 ) &&
            ( TheDevice->Id == 0 );
}

// это последний скрипт
int is_last_script ( const script_cfg_t *Script )
{
    return ( Script->Name == NULL ) &&
            ( Script->Parametr == NULL );
}

/* это последняя задача */
int is_last_task ( const task_cfg_t *Task )
{
    return ( Task->Script == NULL ) &&
            ( Task->Target == NULL );
}

// это последнее устройство связи
int is_last_comm ( const comm_cfg_t *Comm )
{
    return ( Comm->Device == NULL ) &&
            ( Comm->Script == NULL );
}

/**********************************************************************/

// получить описание журнала
static
const config_setting_t* _get_journal ( FILE *Log, const config_t *cfg )
{
    // корневая метка конфигурации журнала
    config_setting_t *setting = config_lookup ( cfg, "Journal" ); // поиск сети
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "В конфигурации отсутствует запись 'Journal'" );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) setting;
}

// получить имя файла журнала ( базы данных )
static
const char* _get_journal_file ( FILE *Log, const config_setting_t* setting )
{
    const char *JnlFile;
    if ( config_setting_lookup_string ( setting, "file", &(JnlFile) ) != CONFIG_TRUE )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "Запись 'Journal' не полная" );
        return ( const char* ) NULL;
    }
    return JnlFile;
}

// получить размер журнала
static
const char* _get_journal_size ( FILE *Log, const config_setting_t* setting, int Verbose )
{
    const char *JnlSize;
    if ( config_setting_lookup_string ( setting, "size", &(JnlSize) ) != CONFIG_TRUE )
    {
        // доп.сообщение
        if ( Verbose )
        {
            askue_config_error ( Log, "Отсутствует запись 'Journal.size'" );
        }
        return ( const char* ) NULL;
    }
    return JnlSize;
}

// установить значение размера журнала
static
size_t _set_journal_size ( FILE *Log, const config_setting_t* setting, int Verbose )
{
    const char *JnlSize = _get_journal_size ( Log, setting, Verbose );
    if ( JnlSize == NULL)
    {
        if ( Verbose )
        {
            askue_config_info ( Log, "Установка значения по умолчанию 'Journal.size = 3'" );
        }
        // значение по умолчанию
        return ( size_t ) 3;
    }
    else
    {
        // значение из конфига
        return (size_t) strtoul ( JnlSize, NULL, 10 );
    }
}

// прочитать кол-во флешбеков
static
const char* _get_journal_flashback ( FILE *Log, const config_setting_t* setting, int Verbose )
{
    const char *JnlFlashback;
    if ( config_setting_lookup_string ( setting, "flashback", &(JnlFlashback) ) != CONFIG_TRUE )
    {
        // доп.сообщение
        if ( Verbose )
        {
            askue_config_error ( Log, "Отсутствует запись 'Journal.flashback'" );
        }
        return ( const char* ) NULL;
    }
    else
    {
        return JnlFlashback;
    }
}

// установить кол-во флешбеков
static
size_t _set_journal_flashback ( FILE *Log, const config_setting_t* setting, int Verbose )
{
    const char *JnlFlashback = _get_journal_flashback ( Log, setting, Verbose );
    if ( JnlFlashback == NULL )
    {
        if ( Verbose )
        {
            askue_config_info ( Log, "Установка значения по умолчанию 'Journal.flashback = 0'" );
        }
        // значение по умолчанию
        return ( size_t ) 0;
    }
    else
    {
        // значение из конфига
        return (size_t) strtoul ( JnlFlashback, NULL, 10 );
    }
}

// чтение конфигурации базы
static
int __config_read_journal ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    // данные о журнале
    const config_setting_t *Jnl = _get_journal ( Log, cfg );
    if ( Jnl == NULL ) return ASKUE_ERROR;
    // файл журнала
    const char *JnlFile = _get_journal_file ( Log, Jnl );
    if ( JnlFile == NULL ) return ASKUE_ERROR;
    // память под журнал
    ACfg->Journal = mymalloc ( sizeof ( journal_cfg_t ) );
    // размер в днях 
    ACfg->Journal->Size = _set_journal_size ( Log, Jnl, Verbose );
    // флешбек в днях
    ACfg->Journal->Flashback = _set_journal_flashback ( Log, Jnl, Verbose );
    // значение из конфига
    ACfg->Journal->File = mystrdup ( JnlFile );
    // доп.сообщение
    if ( Verbose )
        askue_config_info ( Log, "Настройки журнала загружены." );
     
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// поиск описания в конфиге лога
static
const config_setting_t* _get_log ( FILE *Log, const config_t *cfg )
{
    config_setting_t *setting = config_lookup ( cfg, "Log" ); // поиск сети
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "В конфигурации отсутствует запись 'Log'" );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) setting;
}

// получить параметры лога
static
int _get_log_params ( FILE *Log, const config_setting_t *setting, const char **File, const char **Lines, const char **Mode, int Verbose )
{
    if ( config_setting_lookup_string ( setting, "file", File ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Log.file'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( setting, "mode", Mode ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Log.Mode'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( setting, "lines", Lines ) == CONFIG_FALSE )
    {
        if ( Verbose ) 
        {
            askue_config_error ( Log, "Отсутствует запись 'Log.lines'." );
            //askue_config_info ( Log, "Установка значения по умолчанию 'Log.lines = 1000'" );
        }
        *Lines = NULL;
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// установить параметры
static
void _set_log_params ( FILE *Log, askue_cfg_t *ACfg, const char *File, const char *Lines, const char *Mode, int Verbose )
{
    ACfg->Log->File = mystrdup ( File );
    if ( Lines != NULL )
    {
        ACfg->Log->Lines = ( size_t ) strtoul ( Lines, NULL, 10 );
    }
    else
    {
        ACfg->Log->Lines = ( size_t ) 1000;
        if ( Verbose ) 
            askue_config_info ( Log, "Установка значения по умолчанию 'Log.lines = 1000'" );
    }
    ACfg->Log->Mode = mystrdup ( Mode );
}

// чтение конфигурации лога
static
int __config_read_log ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    // корневая структура конфигурации лога
    const config_setting_t *LogSett = _get_log ( Log, cfg );
    if ( LogSett == NULL ) return ASKUE_ERROR;
    // поиск параметров лога
    const char *File, *Lines, *Mode;
    if ( _get_log_params ( Log, LogSett, &File, &Lines, &Mode, Verbose ) == ASKUE_ERROR )
    {
        return ASKUE_ERROR;
    }
    // выделить память 
    ACfg->Log = mymalloc ( sizeof ( log_cfg_t ) );
    // скопировать данные из текстового конфига
    _set_log_params ( Log, ACfg, File, Lines, Mode, Verbose );
    // дополнительное сообщение
    if ( Verbose )
        askue_config_info ( Log, "Настройки лога загружены." );
     
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// поиск описания порта в конфигурации
static
const config_setting_t* _get_port ( FILE *Log, const config_t *cfg )
{
    config_setting_t *setting = config_lookup ( cfg, "Port" );
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "В конфигурации отсутствует запись 'Port'." );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) setting;
}

// поиск параметров в конфиге
static 
int _get_port_params ( FILE *Log,
                        const config_setting_t *port_setting, 
                        const char **File, 
                        const char **DBits, 
                        const char **SBits, 
                        const char **Parity,
                        const char **Speed )
{
    if ( config_setting_lookup_string ( port_setting, "file", File ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Port.file'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "dbits", DBits ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Port.databits'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "sbits", SBits ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Port.stopbits'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "parity", Parity ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Port.parity'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "speed", Speed ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Port.speed'." );
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// установить параметры
static
void _set_port_params ( askue_cfg_t *ACfg, const char *File,
                                             const char *DBits,
                                             const char *SBits,
                                             const char *Parity,
                                             const char *Speed )
{
    // Заполнить память данными
    ACfg->Port->DBits = mystrdup ( DBits );
    ACfg->Port->SBits = mystrdup ( SBits );
    ACfg->Port->Speed = mystrdup ( Speed );
    ACfg->Port->Parity = mystrdup ( Parity );
    ACfg->Port->File = mystrdup ( File );
}

// чтение конфигурации порта
static
int __config_read_port ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    // поиск корневой структуры конфигурации порта
    const config_setting_t *Port = _get_port ( Log, cfg );
    if ( Port == NULL ) return ASKUE_ERROR;
    // параметры порта
    const char *File, *DBits, *SBits, *Parity, *Speed;
    // поиск параметров порта
    if ( _get_port_params ( Log, Port, &File, &DBits, &SBits, &Parity, &Speed ) == ASKUE_ERROR )
        return ASKUE_ERROR;
    // выделить память
    ACfg->Port = mymalloc ( sizeof ( port_cfg_t ) );
    // заполнить параметры
    _set_port_params ( ACfg, File, DBits, SBits, Parity, Speed );
    
    // доп. сообщение 
    if ( Verbose )
        askue_config_info ( Log, "Настройки порта загружены." );
     
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// поиск параметров конкретного устройства
static 
int _get_the_device_param ( FILE *Log,
                             const config_setting_t *next_device, 
                             long int *Id,
                             const char **Name,
                             const char **Type,
                             const char **Timeout, 
                             int Verbose )
{
    if ( config_setting_lookup_int ( next_device, "id", Id ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Devices.id'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( next_device, "name", Name ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Devices.name'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( next_device, "type", Type ) == CONFIG_FALSE )
    {
        askue_config_fail ( Log, "Отсутствует запись 'Devices.type'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( next_device, "timeout", Timeout ) == CONFIG_FALSE )
    {
        if ( Verbose ) 
        {
            askue_config_error ( Log, "Отсутствует запись 'Devices.timeout'." );
        }
        *Timeout = NULL;
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}
            
// установка параметров устройства
static
void _set_the_device_param( FILE *Log,
                             device_cfg_t *Device, 
                             const char *Name, 
                             const char *Type,
                             const char *Timeout,
                             long int Id,
                             int Verbose )
{
    Device->Name = mystrdup ( Name );
    Device->Type = mystrdup ( Type );
    if ( Timeout != NULL )
    {
        Device->Timeout = ( uint32_t ) strtoul ( Timeout, NULL, 10 );
    }
    else
    {
        if ( Verbose )
            askue_config_info ( Log, "Установка значения по умолчанию 'Device.Timeout' = 5000." );
        Device->Timeout = ( uint32_t ) 5000;
    }
    Device->Id = Id;
}
                                                
// разбор описания устройства
static
int __config_read_the_device ( FILE *Log, const config_setting_t *next_device, device_cfg_t *Device, int Verbose )
{
    // поиск обязательных параметров: id, name, type
    const char *Name, *Type, *Timeout;
    long int Id;
    if ( _get_the_device_param ( Log, next_device, &Id, &Name, &Type, &Timeout, Verbose ) == ASKUE_ERROR )
        return ASKUE_ERROR;
    // копировать данные в конфигурацию
    _set_the_device_param ( Log, Device, Name, Type, Timeout, Id, Verbose );
    
    return ASKUE_SUCCESS;
}

// поиск описания списка устройств
static
const config_setting_t* _get_device_list ( FILE *Log, const config_t *cfg )
{
    config_setting_t *setting = config_lookup ( cfg, "Devices" );
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "В конфигурации отсутствует запись 'Devices'." );
        return ( const config_setting_t* ) NULL;
    }
    else
    {
        return ( const config_setting_t* ) setting;
    }
}

// инициализировать конкретное устройство
static
void __config_init_the_device ( device_cfg_t *TheDevice )
{
    TheDevice->Name = NULL;
    TheDevice->Type = NULL;
    TheDevice->Timeout = 0;
    TheDevice->Id = 0;
}

// инициализировать устройства
static
void __config_init_device ( askue_cfg_t *ACfg, size_t DeviceAmount )
{
    for ( size_t i = 0; i < DeviceAmount; i++ )
    {
        __config_init_the_device ( ACfg->Device + i );
    }
}

// чтение конфигурации списка устройств
static
int __config_read_device ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура списка устройств
    const config_setting_t *DeviceList = _get_device_list ( Log, cfg );
    if ( DeviceList == NULL ) return ASKUE_ERROR;
    // кол-во устройств в списке + 1 место под сервер + 1 место под конечный элемент
    size_t Amount = config_setting_length ( DeviceList ) + 2;
    // выделить память
    ACfg->Device = ( device_cfg_t* ) mymalloc ( sizeof ( device_cfg_t ) * ( Amount ) );
    __config_init_device ( ACfg, Amount );
    
    // Заполнить память
    // Нулевое устройство - это АСКУЭ
    ACfg->Device->Name = mystrdup ( "0" );
    ACfg->Device->Type = mystrdup ( "ASKUE" );
    ACfg->Device->Timeout = 0;
    ACfg->Device->Id = 0;
    // остальные устройства
    int Result = ASKUE_SUCCESS; // результат работы
    for ( size_t i = 1; ( i < ( Amount - 1 ) ) && ( Result != ASKUE_ERROR ); i++ )
    {
        // описание устройства
        config_setting_t *next_device = config_setting_get_elem ( DeviceList, i - 1 );
        // разбор описания устройства
        Result = __config_read_the_device ( Log, next_device, ACfg->Device + i, TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) );
    }
    
    if ( ( Result == ASKUE_SUCCESS ) && TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) )
        askue_config_info ( Log, "Устройства загружены." );
    
    return Result;
}

/**********************************************************************/

// поиск описания сети
static
const config_setting_t* _get_network ( FILE *Log, const config_t *cfg )
{
    config_setting_t *Network = config_lookup ( cfg, "Network" );
    if ( Network == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "В конфигурации отсутствует запись 'Network'." );
        return ( const config_setting_t* ) NULL;
    }
    else
    {
        return ( const config_setting_t* ) Network;
    }
}

// инициализация конфигурации сети
static
void __config_init_network ( long int *Network, size_t Size )
{
    for ( size_t i = 0; i < Size; i++ ) Network[ i ] = ( long int ) -1;
    Network[ 0 ] = 0;
}

// читать структу сети
static
int __config_read_network ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура описывающая сеть
    const config_setting_t* Network = _get_network ( Log, cfg );
    if ( Network == NULL ) return ASKUE_ERROR;
    // размер сети + 1 место под оконечный элемент ( -1 )
    size_t Size = config_setting_length ( Network ) + 1;
    // выделить память
    ACfg->Network = mymalloc ( ( Size ) * sizeof ( long int ) );
    // заполнить начальными значениями
    __config_init_network ( ACfg->Network, Size );
    // перебор всех рёбер графа
    for ( size_t i = 0; i < Size - 1; i++ )
    {
        // получить ребро графа
        config_setting_t *edge = config_setting_get_elem ( Network, i );
        // id устройства через которое подключаемся
        int Base = config_setting_get_int_elem ( edge, 0 );
        // id устройства к которому подключаемся
        int Remote = config_setting_get_int_elem ( edge, 1 );
        // запись структуры в конфиг
        ACfg->Network[ Remote ] = ( long int ) Base;
    }
    
    if ( TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) )
    {
        askue_config_info ( Log, "Карта сети загружена." )
    }
    
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// разделение скрипта на имя и параметр
static
void _get_script ( char **ScriptName, char **ScriptParametr, const char *script )
{
    const char *Parametr = strchr ( script, ':' );
    
    if ( Parametr != NULL )
    {
        size_t len = ( size_t ) ( ( const char* ) Parametr - ( const char* ) script );
        *ScriptName = mystrndup ( script, len );
        
        Parametr++;
        while ( *Parametr != '\0' && isspace ( *Parametr ) )
            Parametr++;
        *ScriptParametr = mystrdup ( Parametr );
    }
    else
    {
        *ScriptParametr = NULL;
        *ScriptName = mystrdup ( script );
    }
}

// чтение скрипта и его параметра
static
script_cfg_t __config_script ( const char *script )
{
    char *ScriptName, *ScriptParametr;
    _get_script ( &ScriptName, &ScriptParametr, script );
    return ( script_cfg_t ) { ScriptName, ScriptParametr };
}

/**********************************************************************/

// поиск параметров коммуникации
static
int _get_the_comm_params ( FILE *Log,
                            const config_setting_t *TheComm, 
                            long int *Id,
                            const char **Script )
{
    if ( config_setting_lookup_int ( TheComm, "id", Id ) == CONFIG_FALSE )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "Отсутствует запись 'Communication.id'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( TheComm, "script", Script ) == CONFIG_FALSE )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "Отсутствует запись 'Communication.script'." );
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// прочитать способ коммуникации
static
int __config_read_the_comm ( FILE *Log, const config_setting_t *TheComm, askue_cfg_t *ACfg, size_t Index )
{
    const char *Script;
    long int Id;
    if ( _get_the_comm_params ( Log, TheComm, &Id, &Script ) == ASKUE_ERROR ) return ASKUE_ERROR;
    
    // определить id
    // присвоить коммуникации описание устройства
    comm_cfg_t *Comm = ACfg->Comm + Index;
    
    device_cfg_t *Dev = ACfg->Device;
    while ( !is_last_device ( Dev ) &&
             Dev->Id != Id ) 
        Dev ++;
    
    if ( is_last_device ( Dev ) )
    {
        askue_config_fail ( Log, "Отсутствует соответствующее описание устройства." ); 
        return ASKUE_ERROR;
    }
    
    Comm->Device = Dev;
    // прочитать скрипт для выполнения соединения
    Comm->Script = mymalloc ( sizeof ( script_cfg_t ) );
    *( Comm->Script ) = __config_script ( Script );
    
    return ASKUE_SUCCESS;
}

// поиск списка коммуникаций
static
const config_setting_t* _get_comm ( FILE *Log, const config_t *cfg )
{
    config_setting_t *Communication = config_lookup ( cfg, "Communications" );
    if ( Communication == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "В конфигурации отсутствует запись 'Communications'." );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) Communication;
}

// конфигурировать следующую коммуникацию
static
int __config_next_comm ( FILE *Log, const config_setting_t *Comm, askue_cfg_t *ACfg, size_t offset )
{
    config_setting_t *TheComm = config_setting_get_elem ( Comm, offset );
    return __config_read_the_comm ( Log, TheComm, ACfg, offset );
}

// конфигурирование всех коммуникаций
static 
int __config_each_comm ( FILE *Log, const config_setting_t *Comm, askue_cfg_t *ACfg, size_t CommAmount )
{
    int Result = ASKUE_SUCCESS;
    for ( size_t i = 0; i < CommAmount && Result != ASKUE_ERROR; i++ )
    {
        Result = __config_next_comm ( Log, Comm, ACfg, i );
    }
    return Result;
}

// инициализировать память конкретной конфигурации
static
void __config_init_the_comm ( comm_cfg_t *TheComm )
{
    TheComm->Device = NULL;
    TheComm->Script = NULL;
}

// инициализировать память, выделенную под коммуникации
static
void __config_init_comm ( askue_cfg_t *Cfg, size_t CommAmount )
{
    for ( size_t i = 0; i < CommAmount; i++ )
    {
        __config_init_the_comm ( Cfg->Comm + i );
    }
}

// читать структу коммуникаций
static
int __config_read_comm ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура описывающая коммуникации
    const config_setting_t *Communication = _get_comm ( Log, cfg );
    if ( Communication == NULL ) return ASKUE_ERROR;
    
    // кол-во способов коммуникации + место под последнюю
    size_t Amount = config_setting_length ( Communication ) + 1;
    // выделить память
    ACfg->Comm = mymalloc ( ( Amount ) * sizeof ( comm_cfg_t ) );
    // инициализировать память
    __config_init_comm ( ACfg, Amount );
    // прочитать коммуникации
    if ( __config_each_comm ( Log, Communication, ACfg, Amount - 1 ) == ASKUE_SUCCESS )
    {
        if ( TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) )
        {
            askue_config_info ( Log, "Устройства связи загружены." );
        }
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_ERROR;
    }
}

/**********************************************************************/

static
void __config_init_the_script ( script_cfg_t *Script )
{
    Script->Name = NULL;
    Script->Parametr = NULL;
}

// прочитать способ коммуникации
static
int __config_read_the_task ( FILE *Log, const config_setting_t *Setting, task_cfg_t *Task )
{
    // цель задачи
    Task->Target = mystrdup ( config_setting_name ( Setting ) );
    // кол-во действий ( скриптов ) + 1 место под оконечный
    size_t Amount = config_setting_length ( Setting ) + 1; 
    // выделить память
    Task->Script = mymalloc ( sizeof ( script_cfg_t ) * Amount );
    // инициализация памяти
    for ( size_t i = 0; i < Amount; i++ )
    {
        __config_init_the_script ( Task->Script + i );
    }
    // запись значений в память
    for ( size_t i = 0; i < Amount - 1; i++ )
    {
        // скрипт вместе с параметром
        const char *script = config_setting_get_string_elem ( Setting, i );
        // разбиение на параметр и скрипт
        char *ScriptName, *ScriptParametr;
        _get_script ( &ScriptName, &ScriptParametr, script );
        Task->Script[ i ].Name = ScriptName;
        Task->Script[ i ].Parametr = ScriptParametr;
    }
    
    return ASKUE_SUCCESS;
}

// Найти описание списка задач
static
const config_setting_t* _get_task ( FILE *Log, const config_t *cfg )
{
    config_setting_t *Task = config_lookup ( cfg, "Tasks" );
    if ( Task == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( Log, "В конфигурации отсутствует запись 'Tasks'." );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) Task;
}

// конфигурирование всех задач
static
int __config_each_task ( FILE *Log, const config_setting_t* Setting, task_cfg_t *Task, size_t TaskAmount )
{
    int Result = ASKUE_SUCCESS;
    for ( size_t i = 0; ( i < TaskAmount ) && ( Result == ASKUE_SUCCESS ); i++ )
    {
        config_setting_t *TheSetting = config_setting_get_elem ( Setting, i );
        Result = __config_read_the_task ( Log, TheSetting, Task + i );
    }
    return Result;
}

static
void __config_init_the_task ( task_cfg_t *TheTask )
{
    TheTask->Target = NULL;
    TheTask->Script = NULL;
}

// инициализировать память под задачи
static
void __config_init_task ( askue_cfg_t *ACfg, size_t TaskAmount ) 
{
    for ( size_t i = 0; i < TaskAmount; i++ )
    {
        __config_init_the_task ( ACfg->Task + i );
    }
}

// читать структу задач
static
int __config_read_task ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура описывающая задачи
    const config_setting_t* Task = _get_task ( Log, cfg );
    if ( Task == NULL ) return ASKUE_ERROR;
    // кол-во задач + место под оконечную ( пустую )
    size_t Amount = config_setting_length ( Task ) + 1;
    // выделить память
    ACfg->Task = mymalloc ( ( Amount ) * sizeof ( task_cfg_t ) );
    __config_init_task ( ACfg, Amount );
    // прочитать задачи
    if ( __config_each_task ( Log, Task, ACfg->Task, Amount - 1 ) == ASKUE_SUCCESS )
    {
        if ( TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) )
            askue_config_info ( Log, "Задачи загружены." );
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_ERROR;
    }
}

/**********************************************************************/

// сообщение об ошибке чтения конфига
static
void _say_ConfigReadFail ( FILE *file, const config_t *cfg )
{
    if ( snprintf ( _gBuffer_, _ASKUE_TBUFLEN, "%s at line %d.", config_error_text ( cfg ), config_error_line ( cfg ) ) > 0 )
        askue_config_fail ( file, _gBuffer_ );
}

// сообщение об успешном чтении конфига
static
int _say_ConfigReadOk ( FILE *file )
{
    if ( snprintf ( _gBuffer_, _ASKUE_TBUFLEN, "Файл с конфигурацией '%s' - прочитан.", ASKUE_FILE_CONFIG ) > 0 )
    {
        askue_config_ok ( file, _gBuffer_ );
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_ERROR;
    }
}

// читать все части конфига
static 
int __config_read ( FILE *Log, const config_t *cfg, askue_cfg_t *ACfg )
{
    return __config_read_journal ( Log, cfg, ACfg ) == ASKUE_SUCCESS &&
            __config_read_log ( Log, cfg, ACfg ) == ASKUE_SUCCESS &&
            __config_read_port ( Log, cfg, ACfg ) == ASKUE_SUCCESS && 
            __config_read_device ( Log, cfg, ACfg ) == ASKUE_SUCCESS &&
            __config_read_network ( Log, cfg, ACfg ) == ASKUE_SUCCESS &&
            __config_read_comm ( Log, cfg, ACfg ) == ASKUE_SUCCESS &&
            __config_read_task ( Log, cfg, ACfg ) == ASKUE_SUCCESS;
}

/* Точка чтения конфигурации */
int askue_config_read ( askue_cfg_t *ACfg, FILE *Log )
{
    config_t cfg; // конфиг из файла
	config_init ( &cfg ); // выделить память под переменную с конфигурацией
    
    // текстовый буфер
    memset ( _gBuffer_, '\0', _ASKUE_TBUFLEN );
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    if ( Verbose )
    {
        if ( snprintf ( _gBuffer_, _ASKUE_TBUFLEN, "Открывается файл конфигурации: '%s'.", ASKUE_FILE_CONFIG ) > 0 )
            askue_config_info ( Log, "Конфигурация загружена." );
    }
    // читать конфиг из файла
	if ( config_read_file ( &cfg, ASKUE_FILE_CONFIG ) != CONFIG_TRUE ) // открыть и прочитать файл
	{
        _say_ConfigReadFail ( Log, &cfg );
        config_destroy ( &cfg );
        return ASKUE_ERROR;
    }
    // дополнительное сообщение
    if ( Verbose && ( _say_ConfigReadOk ( Log ) == ASKUE_ERROR ) )
    {
        config_destroy ( &cfg );
        return ASKUE_ERROR;
    }
    // результат работы
    int Result;
    if ( __config_read ( Log, &cfg, ACfg ) )
    {
        askue_config_ok ( Log, "Конфигурация загружена." );
        Result = ASKUE_SUCCESS;
    }
    else
    {
        Result = ASKUE_ERROR;
    }
   
    config_destroy ( &cfg );
    return Result;
}

/**********************************************************************/

/* Инициализация памяти конфигурации */
void askue_config_init ( askue_cfg_t *Cfg )
{
    Cfg->Device = NULL;
    Cfg->Task = NULL;
    Cfg->Comm = NULL;
    Cfg->Port = NULL;
    Cfg->Log = NULL;
    Cfg->Journal = NULL;
    Cfg->Network = NULL;
    Cfg->Flag = 0;
}

// Удаление данных о порте
static
void __destroy_port ( askue_cfg_t *ACfg )
{
    if ( ACfg->Port )
    {
        myfree ( ACfg->Port->DBits );
        myfree ( ACfg->Port->SBits );
        myfree ( ACfg->Port->Speed );
        myfree ( ACfg->Port->File );
        myfree ( ACfg->Port->Parity );
        free ( ACfg->Port );
    }
}

// Удаление данных о логе
static
void __destroy_log ( askue_cfg_t *ACfg )
{
    if ( ACfg->Log )
    {
        myfree ( ACfg->Log->File );
        myfree ( ACfg->Log->Mode );
        free ( ACfg->Log );
    }
}

// Удаление данных о базе
static
void __destroy_journal ( askue_cfg_t *ACfg )
{
    if ( ACfg->Journal )
    {
        myfree ( ACfg->Journal->File );
        myfree ( ACfg->Journal );
    }
}

// удалить информацию об устройстве
static
void __destroy_device_info ( device_cfg_t *TheDevice )
{
    myfree ( TheDevice->Name );
    myfree ( TheDevice->Type );
}

// удалить информацию о списке устройств
static
void __destroy_device ( askue_cfg_t *ACfg )
{
    if ( ACfg->Device )
        for ( device_cfg_t *TheDevice = ACfg->Device; is_last_device ( TheDevice ); TheDevice++ )
        {
            __destroy_device_info ( TheDevice );
        }
    
    myfree ( ACfg->Device );
}

// удалить инфу о скрипте
static 
void __destroy_script_info ( script_cfg_t *TheScript )
{
    myfree ( TheScript->Name );
    myfree ( TheScript->Parametr );
}

// удалить информацию о коммуникации
static
void __destroy_comm_info ( comm_cfg_t *TheComm )
{
    __destroy_script_info ( TheComm->Script );
    myfree ( TheComm->Script );
}

// удалить информацию о списке коммуникаций
static
void __destroy_comm ( askue_cfg_t *ACfg )
{
    if ( ACfg->Comm )
        for ( comm_cfg_t *TheComm = ACfg->Comm; is_last_comm ( TheComm ); TheComm++ )
        {
            __destroy_comm_info ( TheComm );
        }
    
    myfree ( ACfg->Comm );
}

// удалить информацию о задаче
static
void __destroy_task_info ( task_cfg_t *TheTask )
{    
    myfree ( TheTask->Target );
    if ( TheTask->Script )
        for ( script_cfg_t *TheScript = TheTask->Script; is_last_script ( TheScript ); TheScript++ )
        {
            __destroy_script_info ( TheScript );
        }
    myfree ( TheTask->Script );
}

// удалить информацию о списке задач
static
void __destroy_task ( askue_cfg_t *ACfg )
{
    if ( ACfg->Task )
        for ( task_cfg_t *TheTask = ACfg->Task; is_last_task ( TheTask ); TheTask++ )
        {
            __destroy_task_info ( TheTask ); 
        }
    
    myfree ( ACfg->Task );
}

// удалить сеть
static
void __destroy_network ( askue_cfg_t *Cfg )
{
    myfree ( Cfg->Network );
}

/* Деинициализация памяти конфигурации */
void askue_config_destroy ( askue_cfg_t *Cfg )
{
    __destroy_port ( Cfg );
    __destroy_log ( Cfg );
    __destroy_journal ( Cfg );
    __destroy_device ( Cfg );
    __destroy_comm ( Cfg );
    __destroy_task ( Cfg );
    __destroy_network ( Cfg );
}

#undef askue_config_fail
#undef askue_config_error
#undef askue_config_ok
#undef askue_config_info





