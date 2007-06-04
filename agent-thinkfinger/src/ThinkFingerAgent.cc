/* ThinkFingerAgent.cc
 *
 * An agent for ThinkFinger library (access to fingerprint reader)
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id$
 */

#include "ThinkFingerAgent.h"

#define PC(n)       (path->component_str(n))

/*
libthinkfinger_state global_state; // save the last state

s_tfdata swipes; // save the nubmer of (un)succ. swipes globally
*/


//FIXME remove unused parts...
typedef struct {
    char bir[MAX_PATH];
    int swipe_success;
    int swipe_failed;
    ThinkFingerAgent *tfagent;
} s_tfdata;

/**
 * set the current status, for which YaST can query
 */
static void set_status (int swipe_success, int swiped_required, int swipe_failed)
{
    y2milestone ("Please swipe your finger (successful swipes %i/%i, failed swipes: %i)...",
	swipe_success, swiped_required, swipe_failed);
}


//void ThinkFingerAgent::callback (libthinkfinger_state state, void *data)
static void callback (libthinkfinger_state state, void *data)
{
    s_tfdata *tfdata = (s_tfdata *) data;
	switch (state) {
	    case TF_STATE_ACQUIRE_SUCCESS:
		y2milestone (" done - success");
		break;
	    case TF_STATE_ACQUIRE_FAILED:
		y2milestone (" failed");
		break;
	    case TF_STATE_ENROLL_SUCCESS:
		set_status (tfdata->swipe_success, 3, tfdata->swipe_failed);
		y2milestone (" done.\nStoring data (%s)...", tfdata->bir);
		break;
	    case TF_STATE_SWIPE_FAILED:
		y2milestone (" TF_STATE_SWIPE_FAILED");
		set_status (tfdata->swipe_success, 3, ++tfdata->swipe_failed);
		break;
	    case TF_STATE_SWIPE_SUCCESS:
		y2milestone (" TF_STATE_SWIPE_SUCCESS");
		set_status (++tfdata->swipe_success, 3, tfdata->swipe_failed);
		break;
	    case TF_STATE_SWIPE_0:
		y2milestone (" TF_STATE_SWIPE_0");
		set_status (tfdata->swipe_success, 3, tfdata->swipe_failed);
		break;
	    default:
		break;
	}
	/*
	global_state	= state;
	swipes.swipe_failed	= tfdata->swipe_failed;
	swipes.swipe_success	= tfdata->swipe_success;
	*/
	tfdata->tfagent->global_state	= state;
	tfdata->tfagent->swipe_success	= tfdata->swipe_success;
	tfdata->tfagent->swipe_failed	= tfdata->swipe_failed;
}

/**
 * Constructor
 */
ThinkFingerAgent::ThinkFingerAgent() : SCRAgent()
{
    tf		= NULL;
    initialized = false;
    global_state	= TF_STATE_UNDEFINED;
    swipe_success	= 0;
    swipe_failed	= 0;
}

/**
 * Destructor
 */
ThinkFingerAgent::~ThinkFingerAgent()
{
}

/**
 * Dir
 */
YCPList ThinkFingerAgent::Dir(const YCPPath& path) 
{   
    y2error("Wrong path '%s' in Dir().", path->toString().c_str());
    return YCPNull();
}


/**
 * Read
 */
YCPValue ThinkFingerAgent::Read(const YCPPath &path, const YCPValue& arg, const YCPValue& opt) {

    y2debug("Path in Read(): %s", path->toString().c_str());
    YCPValue ret = YCPVoid(); 

    if (!initialized)
    {
	y2error ("ThinkFinger not initialized yet!");
        return ret;
    }

    if (path->length() == 0) {
    	ret = YCPString("0");
    }
    else if (path->length() == 1) {
        
	if (PC(0) == "error") {
	    // return the last error message
	    ret = YCPString ("error_message");
	}
	else if (PC(0) == "state") {
	    YCPMap retmap;
	    if (global_state != TF_STATE_UNDEFINED)
	    {
		retmap->add (YCPString ("swipe_success"), YCPInteger (swipe_success));
		retmap->add (YCPString ("swipe_failed"), YCPInteger (swipe_failed));
		switch (global_state) {
		    case TF_STATE_ACQUIRE_SUCCESS:
			retmap->add (YCPString ("state"), YCPString ("TF_STATE_ACQUIRE_SUCCESS"));
			break;
		    case TF_STATE_ACQUIRE_FAILED:
			retmap->add (YCPString ("state"), YCPString ("TF_STATE_ACQUIRE_FAILED"));
			break;
	    	    case TF_STATE_ENROLL_SUCCESS:
			retmap->add (YCPString ("state"), YCPString ("TF_STATE_ENROLL_SUCCESS"));
			break;
		    case TF_STATE_SWIPE_FAILED:
			retmap->add (YCPString ("state"), YCPString ("TF_STATE_SWIPE_FAILED"));
			break;
		    case TF_STATE_SWIPE_SUCCESS:
			retmap->add (YCPString ("state"), YCPString ("TF_STATE_SWIPE_SUCCESS"));
			break;
		    case TF_STATE_SWIPE_0:
			retmap->add (YCPString ("state"), YCPString ("TF_STATE_SWIPE_0"));
			break;
		    default:
			break;
		}
		global_state	= TF_STATE_UNDEFINED;
	    }
	    return retmap;
	}
	// wait for thread exit
	else if (PC(0) == "exit_status" ) {
	    int *retval;
	    ret = YCPBoolean(false);
	    if ( pthread_join( pt, (void**)&retval ) == 0 ) 
		if ( *retval == 0 )
		    ret = YCPBoolean(true);
	    libthinkfinger_free (tf);
	}
	else {
	    y2error ("Unknown path in Read(): %s", path->toString().c_str());
	}
    }
    else {
       y2error ("Unknown path in Read(): %s", path->toString().c_str());
    }
    return ret;
}


/**
 * Write TODO finalize?
 */
YCPBoolean ThinkFingerAgent::Write(const YCPPath &path, const YCPValue& value,
			     const YCPValue& arg)
{
    y2debug("Path in Write(): %s", path->toString().c_str());
    YCPBoolean ret = YCPBoolean(false);

    if (!initialized) {
        y2error ("ThinkFinger not initialized yet!");
        return ret;
    }

    if (path->length() == 0) {
        ret = YCPBoolean(true);
    }
    return ret;
}
 
/**
 * Execute(.thinkfinger) must be run first to initialize TODO?
 */
YCPValue ThinkFingerAgent::Execute(const YCPPath &path, const YCPValue& val, const YCPValue& arg)
{
    y2debug ("Path in Execute(): %s", path->toString().c_str());
    YCPValue ret = YCPBoolean(false);
    
    if (path->length() == 1) {
 
	if (PC(0) == "add-user") {
	    string username	= "";
	    if (!val.isNull())
	    {
		username = val->asString()->value();
	    }
	    else
	    {
		y2error ("username missing");
		return ret;
	    }
y2internal ("username is '%s'", username.c_str());
// TODO check username length?
// TODO check if user exists? (should be done above...)
	    user	= username;

y2internal ("thinkfinger agent initialization...");
	    libthinkfinger_init_status init_status;
	    tf = libthinkfinger_new (&init_status);
	    if (init_status != TF_INIT_SUCCESS) {
		y2error ("init failed");
		return ret;
	    }
y2internal ("... succeeded");
	    
	    initialized = true;

	    /*
	    s_tfdata tfdata;

	    tfdata.swipe_success	= 0;
	    tfdata.swipe_failed		= 0;
//	    tfdata.tfagent		= this;
	    snprintf (tfdata.bir, MAX_PATH-1, "%s/%s%s", PAM_BIRDIR, username.c_str(), BIR_EXTENSION);
y2internal ("data file is '%s'", tfdata.bir);

	    libthinkfinger_set_file (tf, tfdata.bir);

	    if (libthinkfinger_set_file (tf, tfdata.bir) < 0)
	    {
		y2error ("... set_file failed");
		libthinkfinger_free (tf);
		return ret;//TODO free?
	    }
	    if (libthinkfinger_set_callback (tf, callback, &tfdata) < 0)
	    {
		y2error ("... set_callback failed");
		libthinkfinger_free (tf);
		return ret;//TODO free?
	    }
	    */
	    if (pthread_create (&pt, NULL, (void*(*)(void*))&call_acquire, this) != 0)
	    {
		y2error ("pthread_create failed");
//		libthinkfinger_free (tf);
	    }
	    else
		ret = YCPBoolean (true);

	}	    
    }
    return ret;
}

/**
 * otherCommand
 */
YCPValue ThinkFingerAgent::otherCommand(const YCPTerm& term)
{
    string sym = term->name();

    if (sym == "ThinkFingerAgent") {
        
        return YCPVoid();
    }

    return YCPNull();
}


//void *ThinkFingerAgent::call_acquire (libthinkfinger *my_tf)
void *ThinkFingerAgent::call_acquire (ThinkFingerAgent *ag)
{
    static int retval;
  
    retval	= -1;

    /*
y2internal ("thinkfinger agent initialization...");
	    libthinkfinger_init_status init_status;
	    tf = libthinkfinger_new (&init_status);
	    if (init_status != TF_INIT_SUCCESS) {
		y2error ("init failed");
		return ret;
	    }
y2internal ("... succeeded");
	    
	    initialized = true;

	*/
	    s_tfdata tfdata;

	    tfdata.swipe_success	= 0;
	    tfdata.swipe_failed		= 0;
	    tfdata.tfagent		= ag;
	    snprintf (tfdata.bir, MAX_PATH-1, "%s/%s%s", PAM_BIRDIR, ag->user.c_str(), BIR_EXTENSION);
y2internal ("data file is '%s'", tfdata.bir);

//	    libthinkfinger_set_file (ag->tf, tfdata.bir);

	    if (libthinkfinger_set_file (ag->tf, tfdata.bir) < 0)
	    {
		y2error ("... set_file failed");
//		libthinkfinger_free (ag->tf);
	        pthread_exit((void*)&retval);
	    }
	    if (libthinkfinger_set_callback (ag->tf, callback, &tfdata) < 0)
	    {
		y2error ("... set_callback failed");
//		libthinkfinger_free (ag->tf);
		pthread_exit((void*)&retval);
	    }


y2internal ("acquiring...");
    int tf_state = libthinkfinger_acquire (ag->tf);
y2internal ("acquire done");
    switch (tf_state) {
		case TF_STATE_ACQUIRE_SUCCESS:
			y2internal ("TF_STATE_ACQUIRE_SUCCESS!");
			retval	= 0;
			break;
		case TF_STATE_ACQUIRE_FAILED:
			y2error ("TF_STATE_ACQUIRE_FAILED");
			break;
		case TF_STATE_USB_ERROR:
			y2error ("Could not acquire fingerprint (USB error).");
			break;
		case TF_RESULT_COMM_FAILED:
			y2error ("Could not acquire fingerprint (communication with fingerprint reader failed).");
			break;
		default:
			y2error ("Undefined error occured (%i).", tf_state);
			break;
    }
// TODO it is possible that libthinkfinger_acquire ends without providing TF_STATE_ACQUIRE_SUCCESS/TF_STATE_ACQUIRE_FAILED
    y2internal ("retval is %i", retval);
    pthread_exit((void*)&retval);
}
