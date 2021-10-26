#include "cc_pbx.h"

switch_status_t handle_call(switch_channel_t *channel,char * dsn,switch_mutex_t *mutex,call_details_t *      call_info){

    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_core_session_t* session = switch_channel_get_session(channel);
    const char* caller = switch_channel_get_variable(channel,"sip_from_user");
    const char* callee = switch_channel_get_variable(channel,"sip_req_user");     
    const char* ip=switch_channel_get_variable(channel,"sip_network_ip");
    const char* call_type = switch_channel_get_variable(channel,"call_type");
    const char* fd_id = switch_channel_get_variable(channel,"fd_id");
    const char* cust_id=switch_channel_get_variable(channel,"cust_id");
    const char* transfer = switch_channel_get_variable(channel,"transfer_number");
    const char* rdnis= switch_channel_get_variable(channel,"rdnis");
    char id[20]={'\0'}; 
    char * rng_ton=NULL;
    char * data=NULL; 
    char result[128] = {'\0'};
    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL :%s ,dw%s",transfer,rdnis); 
    switch_channel_set_variable(channel,"application","intercom"); 
    

     if(!get_ext_details(channel,&call_info->caller,dsn,mutex,caller)){
         if(!IS_NULL(call_type)){
          if (!strcmp(call_type,"call_feedback")) {
		struct stack *pt = newStack(5);
		switch_channel_set_variable(channel,"sip_network_ip","119.18.55.154");
		switch_channel_set_variable  (channel,"sip_network_port","5060");
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL :%s",ip);
		call_info->did.dst_id=atoi(fd_id);
		call_info->did.cust_id=(char*)cust_id;
		call_info->did.is_init=1;
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL : %s,%s,%s,%s",cust_id,call_info->did.cust_id, call_type,fd_id);
		handle_ivr(channel, dsn,mutex,call_info,pt,1);
		switch_channel_set_variable(channel,"application","outbound");
                return status;
                }
          }
	if(!verify_did(channel,dsn,mutex,&call_info->did)){
		switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
       		return status;
           }
       
	}
      
     if(!strcmp(rdnis,"")){
        if(strlen(callee)==4){
             callee=switch_mprintf("%s%s",call_info->caller.cust_id,callee);
          }
      if(get_ext_details(channel,&call_info->callee,dsn,mutex,callee)){
        if(call_info->callee.is_rng_ton==1){
        rng_ton= switch_mprintf("select file_path from pbx_prompts where id=%d",call_info->callee.rng_ton_id);
        execute_sql2str(dsn,mutex,rng_ton,result,NELEMS(result));
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"%s id %s\n",rng_ton,result);
        if(strcmp(result,"")){
          data=switch_mprintf("ringback=/var/www/html/pbx/app%s",result);
          switch_core_session_execute_application(session,"set",data);
         }
         else{
          data=switch_mprintf("ringback=%(2000,4000,440.0,480.0)");
          switch_core_session_execute_application(session,"set","ringback=%(2000,4000,440.0,480.0)");
            }
         
         }
        }
         else{
           switch_core_session_execute_application(session,"set","ringback=%(2000,4000,440.0,480.0)");
          }
        }
// should be executed only if caller is extension of system
 if (callee!=NULL){   
   switch(strlen(callee)){
        case 2:
		if(callee[0] != '*'){
                	switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-please_check_number_try_again.wav", NULL);
			switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
		}

		if(call_info->caller.is_sd_allowed == true){
                                  
            	 	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL TYPE : speeddial\n");
                  	   switch_channel_set_variable(channel,"call_type","call_speeddial");
                              switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);
                              handle_sd(channel,dsn,mutex,call_info);
		}else{	// may add prompt feature disabled of something
			switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
		}	
            break;
	case 3:
	     if(callee[0] != '*'){
                        switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-please_check_number_try_again.wav", NULL);
                        switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
			break;
                }

            if(call_info->caller.is_init == true  ){
		    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL TYPE : feature_code\n");
                       switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);

		    feature_code(session, callee,caller, call_info, dsn,mutex);
		      // WHY IS THERE ONLY REJETED HANGUP CAUSE DONT YOU THINK IT CAN HAVE ANOTHER HANGUP CAUSE?
	              switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                  }else{// DO WE NEED ELSE ?
		           switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);

		  }
	        
             break;
        case 4:
             if(call_info->caller.is_init == true){
              callee= switch_mprintf("%s%s",call_info->caller.cust_id,callee); 
                switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL TYPE : SIP extn\n");
                    switch_channel_set_variable(channel,"call_type","sip_extn");
                    switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);
             
                if(!handle_sip_call(channel,dsn,mutex,call_info))
                        status = SWITCH_STATUS_FALSE;
                     // switch_safe_free(call_info->callee.num);

            }
            break;

            /* switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL TYPE : internal exten conf/cg/ivr/queue\n");
       	   // switch_channel_set_variable(channel,"call_type","internal");
              
	    if(verify_internal_exten(channel,dsn,mutex,call_info,callee)){
                	call_info->flags.is_call_internal = true;
	                switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);
          
			if(call_info->cg.is_init){
				handle_cg(channel,call_info);	
			}else if(call_info->conf.is_init){
				handle_conf(channel,call_info);	
			}
	    }else{
	    	 switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-please_check_number_try_again.wav", NULL);
                 switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
		 status = SWITCH_STATUS_FALSE;
	    }*/ 
            break;
	case 6:
                if(callee[0] == '*'){
                      if(verify_internal_exten(channel,dsn,mutex,call_info,callee)){
                        call_info->flags.is_call_internal = true;
                        switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);

                        if(call_info->cg.is_init){
                                handle_cg(channel,call_info,dsn,mutex);
                        }else if(call_info->conf.is_init){
                                handle_conf(channel,call_info);
                        }
                }else{
		
		   switch_channel_answer(channel);  
       		  sleep(1);
                  switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/feature_code_not_available.wav", NULL);
                  switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                  status = SWITCH_STATUS_FALSE;
            }
            break;

                        // switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-please_check_number_try_again.wav", NULL);
                       // switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
			//break;
                }
               break;
           case 5:
	     if(call_info->caller.is_init == true){
		      switch_channel_set_variable(channel,"call_type","call_valetpark");
	             switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);

                        valetpark(session,callee);
		}else{

                   switch_channel_answer(channel);  
                  sleep(1);
                  switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/feature_code_not_available.wav", NULL);
                  switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                  status = SWITCH_STATUS_FALSE;
            }
	    break;
        case 7:
            if(call_info->caller.is_init == true){
            	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL TYPE : SIP extn\n");
                    switch_channel_set_variable(channel,"call_type","sip_extn");
                    switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);

		if(!handle_sip_call(channel,dsn,mutex,call_info))
			status = SWITCH_STATUS_FALSE;
	    }	
            break;
        default:
	    if(verify_did(channel,dsn,mutex,&call_info->did)){
            	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"Dialed Number is DID. %d\n",call_info->caller.is_init);
               
                  if(call_info->caller.is_init == false  ){ 
		     sprintf(id,"%d",call_info->did.id);
                     switch_channel_set_variable(channel,"cust_id",call_info->did.cust_id);
                     switch_channel_set_variable(channel,"application","inbound");
		     switch_channel_set_variable(channel,"callee","inbound");
                     switch_channel_set_variable(channel,"sell_rate",call_info->did.selling_rate);
                     switch_channel_set_variable(channel,"billing_type",call_info->did.bill_type);
                     switch_channel_set_variable(channel,"connect_chrg",call_info->did.conn_charge );
                     switch_channel_set_variable(channel,"provider_id",id );
                     switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"Dialed Number is DID. %s\n",id);
                    
                  	handle_did_dest(channel,dsn,mutex,call_info);
			break;
		}
	    }  
	    if(call_info->caller.is_outbound_allowed){
                      switch_channel_set_variable(channel,"call_type","outbound");
                      switch_channel_set_variable(channel,"application","outbound");		   
                      switch_channel_set_variable(channel,"cust_id",call_info->caller.cust_id);
 
		    call_info->flags.is_call_outbound = true;
			outbound(session,dsn,mutex,call_info,callee);

		if(call_info->caller.is_recording_allowed){
                    
		    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"Dialed Number is DID. %s\n",call_info->callee.num);
        	    set_recording(channel,"call",call_info,dsn,mutex);
	        }
		//outbound(session,dsn,mutex,call_info,callee);
		if(call_info->obd.gw_id != 0||atoi(call_info->mnt.gw_id)!=0){	// may add prompt that no gw found
                     
                   /*  switch_channel_set_variable(channel,"sell_rate",call_info->obd.sell_rate);
                    switch_channel_set_variable(channel,"selling_min_duration",call_info->obd.min_dur);
                     switch_channel_set_variable(channel,"selling_billing_block",call_info->obd.incr_dur);
	               switch_channel_set_variable(channel,"call_plan_id",call_info->obd.call_plan_id);
                       switch_channel_set_variable(channel,"buy_rate",call_info->obd.buy_rate);
                        switch_channel_set_variable(channel,"gateway_group_id",call_info->obd.gw_id);
                         switch_channel_set_variable(channel,"dial_prefix",call_info->obd.dial_prefix);*/

                 	//	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,call_info->obd.sell_rate);
        		bridge_call(session,call_info,callee);
		}

	    }else {   
		   switch_channel_answer(channel);  
                         
                  switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/dialout_disable.wav", NULL);
                  switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                 	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"OUTBOUND DISABLED FOR %s\n",call_info->caller.num);
	 	} 
            break;
      }
     
    if(!IS_NULL(call_type)){
        if (!strcmp(call_type,"call_feedback"))	{
		 struct stack *pt = newStack(5);

		handle_ivr(channel, dsn,mutex,call_info,pt,1);
			
		}
  			}
}else{
      if ((transfer!=NULL) && (strlen(transfer)==4 || strlen(transfer)==7)){ 
    /*switch_core_session_execute_application(session,"bridge","{absolute_codec_string='G722,G729,iLBC,GSM,OPUS,PCMA,PCMU,VP8',originate_timeout=60}sofia/internal/3339912@119.18.55.154:5060");*/
   	callee=transfer;
        handle_sip_call(channel,dsn,mutex,call_info);
	switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
   	status = SWITCH_STATUS_FALSE;
   }
   else if (((transfer!=NULL) && strlen(transfer)>7) ){
 	callee=transfer;
        call_info->flags.is_call_outbound=true; 
	outbound(session,dsn,mutex,call_info,callee);
	bridge_call(session,call_info,callee);
	switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
        status = SWITCH_STATUS_FALSE;
		}
}


    return status;
}



bool handle_sip_call(switch_channel_t *channel,char * dsn,switch_mutex_t *mutex,call_details_t *call) {
        const char* callee = switch_channel_get_variable(channel,"sip_req_user");     
	const char* transfer = switch_channel_get_variable(channel,"transfer_number");
	switch_core_session_t* session = switch_channel_get_session(channel);
	const char* dialstatus = NULL;
	//const char* rdnis=NULL;
        const char* rdnis= switch_channel_get_variable(channel,"rdnis");
        switch_channel_set_variable(channel,"call_type","sip_extn");
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"mobile:%s\n%s\n ",transfer,callee);
        switch_channel_set_variable(channel,"sms","1");
        /*switch_channel_set_variable(channel,"mobile",call->callee.mobile);*/
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"mobile:%s\n",rdnis);
        if(strcmp(rdnis,"")){
	  callee=transfer;
		}
       
        if(strlen(callee)==4){	
             callee=switch_mprintf("%s%s",call->caller.cust_id,callee);
          }
                // dialstatus = switch_channel_get_variable(channel,"DIALSTATUS");
 
   
	if(call->flags.is_call_speeddial||call->flags.is_inbound_sip ){
		callee = call->callee.num;	
	}

        if(!get_ext_details(channel,&call->callee,dsn,mutex,callee)){
                switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"CALL TYPE : OUTBOUND\n");
                switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-please_check_number_try_again.wav", NULL);
                switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                return false;
        }
         switch_channel_set_variable(channel,"sms",call->callee.is_sms_allowed);
        switch_channel_set_variable(channel,"mobile",call->callee.mobile);
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"mobile:%s\n sms:%s\n ",call->callee.mobile,call->callee.is_sms_allowed);
	
        if((call->caller.cust_id!=NULL)&& strcmp(call->callee.cust_id,call->caller.cust_id)){
              sleep(2);
                switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/not_authorized_to_dial.wav", NULL);
                switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                return false;
		} 
    // Handle DND
        if(call->callee.is_dnd == true){
	         sleep(2);	
                switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-dnd_activated.wav", NULL);
                switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                return false;
        }
      // Handle Blacklisted number
        if( (call->callee.blacklist==true) && (is_black_listed(channel,dsn,mutex,call)) ){
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session),SWITCH_LOG_ERROR,"Dialed extension/Number is blacklisted.\n");
                switch_core_session_execute_application(session,"sleep","2000");
		switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/call_blacklist.wav", NULL);
                switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                return false;
        }
               switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session),SWITCH_LOG_ERROR,"Dialed extension/Number is blacklisted.%d\n",call->caller.blacklist);

           if( (call->caller.blacklist==true) && (is_black_list_outgoing(channel,dsn,mutex,call)) ){
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session),SWITCH_LOG_ERROR,"Dialed extension/Number is blacklisted.\n");
                switch_core_session_execute_application(session,"sleep","2000");
		switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/call_blacklist.wav", NULL);
                switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                return false;
        }

        // Handle call recording
        if(call->caller.is_recording_allowed == true||call->did.is_recording_on==true){
               if(call->did.is_recording_on)
	        switch_channel_set_variable(channel,"sip_req_user",call->callee.num);
		 set_recording(channel,"call",call,dsn,mutex);
        }
	// Handle call frwd 
        if(call->callee.is_call_frwd == true){
		check_call_frwd(channel,dsn,mutex,call);
		if(call->frwd[0].type != 0){
			call->flags.is_frwd_all = true;
			forward_call(channel,dsn,mutex,call,0);
			return true;
            }		
                }
         
        bridge_call(session,call,call->callee.num);
        dialstatus = switch_channel_get_variable(channel,"DIALSTATUS");
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Hangup causeis: %s\n",dialstatus);
        /*if((call->callee.is_fmfm == true && call->callee.is_call_frwd==false)&&((dialstatus!=NULL)&&!strcmp(dialstatus,"UNALLOCATED_NUMBER"))){
               handle_fmfm(channel, dsn,mutex, call);

             }*/

       if(call->callee.is_call_frwd ){
        int type = -1;
        if( (dialstatus!=NULL) &&  !strcmp(dialstatus,"BUSY") ){
                type = 1;
	    	
             if(call->frwd[1].type)
                switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/call_rejected.wav", NULL);
        }else if( (dialstatus!=NULL) && !strcmp(dialstatus,"NOANSWER")  ){
                type = 2 ;
	         switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"mobile play debug:\n ");
                 if(call->frwd[2].type)
                   switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-no_user_response.wav", NULL);
        }else if( (dialstatus!=NULL) && !strcmp(dialstatus,"UNALLOCATED_NUMBER")  ){
                type = 3;
	            
                 if(call->frwd[3].type)
                      switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-unallocated_number.wav", NULL);
        }
           
        forward_call(channel,dsn,mutex,call,type);
    }
	
        dialstatus = switch_channel_get_variable(channel,"DIALSTATUS");
        if(  (dialstatus!=NULL) && !strcmp(dialstatus,"BUSY")  ){
             switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," cause1: %d\n",!strcmp(dialstatus,"BUSY") );
             switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/call_rejected.wav", NULL);
        }else if( dialstatus!=NULL  && !strcmp(dialstatus,"NOANSWER") ){
                 switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-no_user_response.wav", NULL);
        }else if( dialstatus!=NULL  &&!strcmp(dialstatus,"UNALLOCATED_NUMBER")  ){
                    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," cause: %s\n",dialstatus);
                      switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/ivr-unallocated_number.wav", NULL);
        }

     
      // switch_channel_set_variable(channel,"sms",call->callee.is_sms_allowed);
      // switch_channel_set_variable(channel,"mobile",call->callee.mobile);
      // switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"mobile:%s\n sms:%s\n ",call->callee.mobile,call->callee.is_sms_allowed);
       if((call->callee.is_fmfm == true )&&((dialstatus!=NULL)&&!strcmp(dialstatus,"UNALLOCATED_NUMBER"))){
               handle_fmfm(channel, dsn,mutex, call);

             }
	

       if((call->callee.is_vm_on == true)&&(strcmp(dialstatus,"SUCCESS"))){
             switch_channel_set_variable(channel,"ann_pmt",call->callee.ann_pmt);

		voicemail(session,NULL,NULL,callee);
	}
        return true;
}

bool handle_sd(switch_channel_t *channel,char * dsn,switch_mutex_t *mutex,call_details_t *call) {
	const char* callee = switch_channel_get_variable(channel,"sip_req_user");
        switch_core_session_t* session = switch_channel_get_session(channel);
	char *query = NULL;
	unsigned int country_id = 0;
	char dial_num[20] = {'\0'};
    	char result[128] = {'\0'};
	query = switch_mprintf("SELECT CONCAT_WS('#',number,country_id) from pbx_speed_dial WHERE customer_id = %s and extension_id = %d  and status='1'  AND digit='%s'",call->caller.cust_id,call->caller.id,callee);
	execute_sql2str(dsn,mutex,query,result,NELEMS(result));
    	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"%s\n",query);
    	switch_safe_free(query);
	
	//if(!IS_NULL(result) && strcmp(result,"")){
	  if(strcmp(result,"")){
        	//sscanf(result,"%s#%u",dial_num,&country_id);
        	sscanf(result,"%[^#]#%u",dial_num,&country_id);
        	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"result :: %s, country : %d , num : %s \n",result,country_id,dial_num);    
	    		
		if(country_id != 0){
			if(call->caller.is_outbound_allowed){
		    		call->flags.is_call_outbound = true;
				if(call->caller.is_recording_allowed){
        	   			 set_recording(channel,"call",call,dsn,mutex);
	        		}//maybe add prompt that not allwed 
				outbound(session,dsn,mutex,call,dial_num);
				if(call->obd.gw_id != 0){	// may add prompt that no gw found
        				bridge_call(session,call,dial_num);
				}
	    		} 
		}else{
		    	call->flags.is_call_speeddial = true;
			strcpy(call->callee.num,dial_num);
		        switch_channel_set_variable (channel,"sip_to_user",call->callee.num);
        		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session),SWITCH_LOG_ERROR,"%s %s\n",dial_num,call->callee.num);
			handle_sip_call(channel,dsn,mutex,call);
		}
    	}else {
        	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session),SWITCH_LOG_ERROR,"Extension details not found for speed dial.\n");
		switch_ivr_play_file(session, NULL, "/home/cloudconnect/pbx_new/upload/def_prompts/speed_dial_not_assign.wav", NULL);
		return false;
    	}
	return true;
}

static int sta_cg_init_callback(void *parg,int argc,char **argv,char **column_names){
    sta_cg_details_t *sta = (sta_cg_details_t*) parg;
    if(argc <2){
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," STA  CALLBACK ERROR : NO DATA %d\n",argc);
        return -1;
    }
    for(int i = 0 ; i<argc;i++)
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"INTERNAL STA %d %s %s\n",i,column_names[i],argv[i]);

    sta->is_init = true;

    if(!IS_NULL(argv[0]) && strlen(argv[0])){
     sta->id             = strdup(argv[0]);
     }

  if(!IS_NULL(argv[1]) && strlen(argv[1])){
     sta->agnt              = strdup(argv[1]);
     }
     sta->rcd  =atoi(argv[2]);

    return 0;
}


int  handle_cg_stcky_agnt(switch_channel_t *channel,char * dsn,switch_mutex_t *mutex,call_details_t *call){


          switch_core_session_t* session = switch_channel_get_session(channel);
        const char* caller = switch_channel_get_variable(channel,"sip_from_user");
        const char* opsp_port = switch_channel_get_variable(channel,"sip_network_port");
        const char* dialstatus;
        char *contact=NULL;
        char *tmp_str;
        char * bs_ivr="/home/cloudconnect/pbx_new/upload/def_prompts/basic_sticky_agent.wav";
        char sticky_buffer[12];
         
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"Dial_string : %s\n,%s",call->caller.cust_id,call->did.cust_id);
        contact=switch_mprintf ("SELECT cg.id,fd.forward,cg.recording FROM `pbx_feedback` as fd, `pbx_callgroup` as cg where fd.ref_id=cg.id and fd.ref_id='%s' and fd.customer_id='%s' and cg.sip like concat('%%',fd.forward,'%%') and fd.src=%s and cg.sticky_agent='1'    ORDER BY fd.hangup_time desc limit 1 ",call->cg.id,call->caller.cust_id,caller);

        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"Dial_string : %s\n",contact);
        execute_sql_callback(dsn,mutex,contact,sta_cg_init_callback,&call->sta);

        if(!IS_NULL(call->sta.agnt)){
        switch_play_and_get_digits(session,1,1,1,15000,"#",bs_ivr,NULL,NULL,sticky_buffer,sizeof(sticky_buffer),"[0-9]+",2000,NULL);
                               switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"Dial_string : %s\n",sticky_buffer);
        if(!strcmp(sticky_buffer,"1")){

                for(int i=0;i<3;i++){
                        switch_channel_set_variable(channel,"call_type","call_cg_sticky_agent");
                        switch_channel_set_variable(channel,"ref_id",call->sta.id);


                        tmp_str = switch_mprintf("{originate_timeout=40}sofia/internal/%s@119.18.55.154:%s",call->sta.agnt,opsp_port);
                        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"Dial_string : %s\n",tmp_str);
                        switch_core_session_execute_application(session,"bridge",tmp_str);
                        if(call->sta.rcd){
                          call->caller.cust_id=strdup(call->did.cust_id);
                          strcpy(call->caller.num,call->sta.agnt);
                          switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"Dial_strin");
                           set_recording(channel,"queue",call,dsn,mutex);
                         }

                        dialstatus =switch_channel_get_variable(channel,"DIALSTATUS");
                        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"Dial_string : %s\n",dialstatus);
                        
                         if(  (dialstatus!=NULL) && !strcmp(dialstatus,"BUSY")  ){
                                switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," cause1: %d\n",!strcmp(dialstatus,"BUSY") );
                                break;
                        }else if( dialstatus!=NULL  && !strcmp(dialstatus,"NOANSWER") ){
                                sleep(10);
                                continue;
                               }

                                
                         if( dialstatus!=NULL  &&!strcmp(dialstatus,"UNALLOCATED_NUMBER")  ){
                                 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," cause: %s\n",dialstatus);
                                 break;
                                }
                        else if( dialstatus!=NULL  &&!strcmp(dialstatus,"SUCCESS")  ){
                                 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR," causefefefe: %s\n",dialstatus);
                                 switch_channel_hangup(channel,SWITCH_CAUSE_CALL_REJECTED);
                                

                               return 1;

                                }

                         }
                       }
                    }
                return 0;
              }


void handle_cg(switch_channel_t* channel,call_details_t *call,char * dsn,switch_mutex_t *mutex){
 	switch_core_session_t* session = switch_channel_get_session(channel);
	const char* opsp_ip = switch_channel_get_variable(channel,"sip_network_ip");
    	const char* opsp_port = switch_channel_get_variable(channel,"sip_network_port");
	char *dial_num = NULL;
        char *sign = (call->cg.grp_type == 1)?",":"|";
	char *token,*rest,*new_str,*tmp;
	token = rest = new_str = tmp = NULL;	
	rest = call->cg.extensions;
        if(handle_cg_stcky_agnt(channel,dsn,mutex,call)){
                        return ;
                }
        switch_channel_set_variable(channel,"call_type","call_group");
        switch_channel_set_variable(channel,"ref_id",call->cg.id);
	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"Bridge string:: CALL GROUP\n");    

	token = strtok_r(call->cg.extensions,",",&rest);

	while( token != NULL){
		dial_num = switch_mprintf("[leg_timeout=%d]sofia/internal/%s@%s:%s",call->cg.ring_timeout,token,opsp_ip,opsp_port);
		if(tmp == NULL){
			new_str = switch_mprintf("%s",dial_num);
		}else{	
               		 new_str = switch_mprintf("%s%s %s",tmp,sign,dial_num);
		}
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"Bridge string:: %s\n",dial_num);    
                switch_safe_free(dial_num);
                switch_safe_free(tmp);
		tmp = new_str;
		token = strtok_r(NULL,",", &rest);
	}
	if(call->cg.is_recording_on){
               set_recording(channel,"cg",call,dsn,mutex);
        }
        switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"Bridge string:: %s \n",new_str);                  
	switch_core_session_execute_application(session,"bridge",new_str);
       	switch_safe_free(new_str);
}

void handle_conf(switch_channel_t * channel,call_details_t *call){
      switch_core_session_t* session = switch_channel_get_session(channel);
	char *new_str = NULL;
               char *tmp_str=NULL;
		switch_time_exp_t tm;
    		char date[50] = "";
    		switch_size_t retsize;
    		switch_time_t ts;
                switch_channel_set_variable(channel,"call_type","call_conference");
                ts = switch_time_now();
    		switch_time_exp_lt(&tm, ts);
    		switch_strftime(date,&retsize,sizeof(date),"%Y-%m-%d-%T",&tm);
                switch_channel_answer(channel);
                tmp_str=switch_mprintf("/var/www/html/pbx/app/%s",call->conf.wc_prompt);
                 switch_ivr_play_file(session, NULL,tmp_str , NULL);

               new_str = switch_mprintf("%s_%s@%s_%s",call->conf.cust_id,call->conf.ext,call->conf.cust_id,call->conf.ext);
               switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"Dialed Number is DID. %s\n",new_str);

                switch_core_session_execute_application(session,"conference",new_str);


                switch_safe_free(new_str);



}    
