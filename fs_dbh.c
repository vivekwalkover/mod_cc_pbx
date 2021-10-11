#include <switch.h>

static switch_cache_db_handle_t *get_db_handle(char *odbc_dsn){

    switch_cache_db_handle_t *dbh = NULL;
    char *dsn;

    if (!zstr(odbc_dsn)) {
        dsn = odbc_dsn;
    } else {
        return NULL;
    }

    if (switch_cache_db_get_db_handle_dsn(&dbh, dsn) != SWITCH_STATUS_SUCCESS) {
        dbh = NULL;
    }

    return dbh;
}

switch_status_t execute_sql(char *odbc_dsn, char *sql, switch_mutex_t *mutex){
    switch_cache_db_handle_t *dbh = NULL;
    switch_status_t status = SWITCH_STATUS_FALSE;

    if (mutex) {
        switch_mutex_lock(mutex);
    }

    if (!(dbh = get_db_handle(odbc_dsn))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB\n");
        goto end;
    }
    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"DBH FOUND");
    status = switch_cache_db_execute_sql(dbh, sql, NULL);

  end:

    switch_cache_db_release_db_handle(&dbh);

    if (mutex) {
        switch_mutex_unlock(mutex);
    }                                                                                                             

    return status;
}

char *execute_sql2str(char *odbc_dsn, switch_mutex_t *mutex, char *sql, char *resbuf, size_t len)
{
    switch_cache_db_handle_t *dbh = NULL;

    char *ret = NULL;

    if (mutex) {
        switch_mutex_lock(mutex);
    }

    if (!(dbh = get_db_handle(odbc_dsn))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB\n");
        goto end;
    }

    ret = switch_cache_db_execute_sql2str(dbh, sql, resbuf, len, NULL);

end:

    switch_cache_db_release_db_handle(&dbh);
                                                                                                                  
    if (mutex) {
        switch_mutex_unlock(mutex);
    }

    return ret;
}

switch_bool_t execute_sql_callback(char *odbc_dsn, switch_mutex_t *mutex, char *sql,
        switch_core_db_callback_func_t callback,void *pdata)
{
    switch_bool_t ret = SWITCH_FALSE;
    char *errmsg = NULL;
    switch_cache_db_handle_t *dbh = NULL;                                                                         

    if (mutex) {
        switch_mutex_lock(mutex);
    }

    if (!(dbh = get_db_handle(odbc_dsn))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB\n");
        goto end;
    }

    switch_cache_db_execute_sql_callback(dbh, sql, callback, pdata, &errmsg);

    if (errmsg) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "SQL ERR: [%s] %s\n", sql, errmsg);
        free(errmsg);
    }
  end:

    switch_cache_db_release_db_handle(&dbh);

    if (mutex) {
        switch_mutex_unlock(mutex);
    }                                                                                                             

    return ret;
}

