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

//FIXME remove unused parts...
typedef struct {
    char bir[MAX_PATH];
    int write_fd;
} s_tfdata;

/**
 * callback function to be called from libthinkfinger_acquire
 * @param state current device status
 * @param data void pointer to user data
 */
static void callback (libthinkfinger_state state, void *data)
{
    s_tfdata *tfdata = (s_tfdata *) data;
    if (write (tfdata->write_fd, &state, sizeof (libthinkfinger_state)) == -1)
	y2error("write to pipe failed: %d (%m)", errno);
}

/**
 * Constructor
 */
ThinkFingerAgent::ThinkFingerAgent() : SCRAgent()
{
    child_pid		= -1;
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

    y2debug ("Path in Read(): %s", path->toString().c_str());
    YCPValue ret = YCPVoid(); 

    if (path->length() == 0) {
    	ret = YCPString("0");
    }
    else if (path->length() == 1) {
        
	if (PC(0) == "error") {
	    // return the last error message
	    ret = YCPString ("error_message");
	}
	else if (PC(0) == "state") {
	    if (!child_pid)
	    {
		y2error ("ThinkFinger not initialized yet!");
		return ret;
	    }
	    YCPMap retmap;
	    int state;
	    size_t size	= sizeof (libthinkfinger_state);
	    int retval = read (data_pipe[0], &state, size);
	    if (retval == -1)
	    {
		if (errno != EINTR && errno != EAGAIN)
		    y2error ("error reading from pipe: %d (%m)", errno);
	    }
	    else {
		retmap->add (YCPString ("state"), YCPInteger (state));
y2internal ("retval from read %d, state is %d", retval, state);
	    }
	    return retmap;
	}
	// wait for child exit
	else if (PC(0) == "exit_status" ) {
y2internal ("waiting for child exit...");
	    int status;
	    int retval	= 255;
	    wait (&status);
	    if (WIFEXITED (status))
		retval	= WEXITSTATUS (status);
	    ret = YCPInteger (retval);
	    close (data_pipe[0]); // close FD for reading
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
    y2milestone ("Path in Execute(): %s", path->toString().c_str());
    YCPValue ret = YCPBoolean(false);
    
    if (path->length() == 1) {
 
	if (PC(0) == "cancel") {
y2internal ("killing child process with pid %d", child_pid);
	    if (child_pid)
		kill (child_pid, 15);
	    // TODO wait and kill -9?
	    child_pid	= -1;
	    ret = YCPBoolean (true);
	}
	else if (PC(0) == "add-user") {
	    string user;
	    if (!val.isNull())
	    {
		user = val->asString()->value();
	    }
	    else
	    {
		y2error ("username is missing");
		return ret;
	    }
	    if (pipe (data_pipe) == -1) {
		y2error ("pipe creation failed");
		return ret;
	    }
	    long arg;
	    arg = fcntl (data_pipe[0], F_GETFL);
	    if (fcntl (data_pipe[0], F_SETFL, arg | O_NONBLOCK ) < 0)
	    {
		y2error ("Couldn't set O_NONBLOCK: errno=%d: %m", errno);
		close (data_pipe[0]);
		close (data_pipe[1]);
		return ret;
	    }
	    child_pid	= fork ();
	    if (child_pid == -1)
	    {
		y2error ("fork failed");
		return ret;
	    }
	    else if (child_pid == 0)
	    {
		// child: call the acquire function, callback gives the
		// actual information to parent
		
		close (data_pipe[0]); // close the read-only FD

		static int retval	= 255;
		s_tfdata tfdata;
		tfdata.write_fd		= data_pipe[1];

		libthinkfinger_init_status init_status;
		libthinkfinger *tf;
		tf = libthinkfinger_new (&init_status);
		if (init_status != TF_INIT_SUCCESS) {
		    y2error ("libthinkfinger_new failed");
		    exit (retval);//FIXME handle sigchld
		}

		snprintf (tfdata.bir, MAX_PATH-1, "%s/%s%s", PAM_BIRDIR, user.c_str(), BIR_EXTENSION);
y2internal ("data file is '%s'", tfdata.bir);//FIXME
// 1. create bir.file in tmpdir
// 2. move it (and possible rename) in Write
// 3. remove the old bir file if present in PAM_BIRDIR with org_uid
// (4. but do not remove the new one of added user of the same name!)

		if (libthinkfinger_set_file (tf, tfdata.bir) < 0)
		{
		    y2error ("libthinkfinger_set_file failed");
		    libthinkfinger_free (tf);
		    exit (retval);		
		}
		if (libthinkfinger_set_callback (tf, callback, &tfdata) < 0)
		{
		    y2error ("libthinkfinger_set_callback failed");
		    libthinkfinger_free (tf);
		    exit (retval);		
		}
		int tf_state = libthinkfinger_acquire (tf);
		y2milestone ("acquire done with state %d", tf_state);
		close (data_pipe[1]);
		libthinkfinger_free (tf);
		exit (tf_state);		
	    }
	    else // parent -> return
	    {
		close (data_pipe[1]); // close FD for writing
		ret = YCPBoolean(true);
	    }
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

