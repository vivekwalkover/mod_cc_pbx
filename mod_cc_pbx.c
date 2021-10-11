/*
 * Freeswitch modular soft-switch application
 * All Rights Reserved © 2019 CloudConnect Communication Pvt. Ltd.
 *
 * The Initial Developer of the Original Code is
 * Ravindrakumar D. Bhatt <ravindra@cloud-connect.in>.
 *
 * mod_cc_pbx.c --Implements broadcast based conference APP for outbound
 * trunk dailing
 *
 */

#include "cc_pbx.h"

#define xml_safe_free(_x) if (_x) switch_xml_free(_x); _x = NULL

SWITCH_MODULE_LOAD_FUNCTION(mod_cc_pbx_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cc_pbx_shutdown);
SWITCH_MODULE_DEFINITION(mod_cc_pbx, mod_cc_pbx_load, mod_cc_pbx_shutdown, NULL);

static const char *global_cf = "cc_pbx.conf";

static struct {
    char *odbc_dsn;
    switch_mutex_t *mutex;
    switch_memory_pool_t *pool;
} globals;

static switch_status_t load_config(void){
    switch_xml_t xml,cfg,settings,param;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if(!(xml = switch_xml_open_cfg(global_cf,&cfg,NULL))){
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Open if %s failed.\n",global_cf);
        return SWITCH_STATUS_TERM;
    }

    if((settings = switch_xml_child(cfg,"settings"))){
       for(param = switch_xml_child(settings,"param");param;param = param->next){
            char *var = (char *) switch_xml_attr_soft(param, "name");
            char *val = (char *) switch_xml_attr_soft(param, "value");
            if(!strcasecmp(var,"odbc-dsn")){
                globals.odbc_dsn = strdup(val);
            }else{
                status = SWITCH_STATUS_TERM;
            }
       } 
    }

    xml_safe_free(xml);
    return status;
}


SWITCH_STANDARD_APP(cc_pbx_function){
    

    switch_status_t status = SWITCH_STATUS_SUCCESS;
    const char* dialstatus;
    switch_channel_t * channel = switch_core_session_get_channel(session);
    //switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;
 
     //call_details_t call = {0};
    call_details_t call = {{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}};

    if( ( status = handle_call(channel,globals.odbc_dsn,globals.mutex,&call) ) != SWITCH_STATUS_SUCCESS ){
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session),SWITCH_LOG_ERROR,"INVALID CALL\n");
	switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
        return ;
    }

    // if callee is our  systems extension than only execute below logic for other type goto end
    dialstatus = switch_channel_get_variable(channel,"DIALSTATUS");
    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Hangup cause: %s\n",dialstatus);     

  
    if(call.frwd[0].type != 0){  
        if(  (dialstatus!=NULL) && !strcmp(dialstatus,"BUSY")  ){
             switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," cause1: %d\n",!strcmp(dialstatus,"BUSY") );
             switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/call_rejected.wav", NULL);
        }else if( dialstatus!=NULL  && !strcmp(dialstatus,"NOANSWER") ){
                 switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-no_user_response.wav", NULL);
        }else if( dialstatus!=NULL  &&!strcmp(dialstatus,"UNALLOCATED_NUMBER")  ){
                    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," cause: %s\n",dialstatus);
                      switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-unallocated_number.wav", NULL);
        }


        switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);   
  } 
        //switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);   
	return;
}


SWITCH_MODULE_LOAD_FUNCTION(mod_cc_pbx_load)
{
    switch_application_interface_t *app_interface;
//  switch_api_interface_t *commands_api_interface;
    switch_status_t status;

    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    memset(&globals,0,sizeof(globals));
    globals.pool = pool;

    switch_mutex_init(&globals.mutex,SWITCH_MUTEX_NESTED,globals.pool);

    if( (status = load_config()) != SWITCH_STATUS_SUCCESS ){
        return status;
    }

    SWITCH_ADD_APP(app_interface, "cc_pbx", "cc_pbx", "cc pbx application", cc_pbx_function, NULL, SAF_NONE);

    return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cc_pbx_shutdown)
{
    switch_safe_free(globals.odbc_dsn);
    switch_mutex_destroy(globals.mutex);
    return SWITCH_STATUS_SUCCESS;
}

