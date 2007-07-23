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

#define INIT_FAILED		500
#define SET_FILE_FAILED		600
#define SET_CALLBACK_FAILED	700

// structure to be passed to callback
typedef struct {
    int write_fd;
} s_tfdata;

static bool child_exited	= false;

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

// handler for SIGCHLD signal (child exited)
static void catch_child_exit (int sig_num)
{
    child_exited	= true;
}

/**
 * Constructor
 */
ThinkFingerAgent::ThinkFingerAgent() : SCRAgent()
{
    child_pid		= -1;
    child_exited	= false;
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
	    else if (retval == size) {
		if (state == SET_FILE_FAILED || state == SET_CALLBACK_FAILED)
		{
		    y2warning ("some initialization failed (%d)...", state);
		    return ret;
		}
		retmap->add (YCPString ("state"), YCPInteger (state));
	    }
	    if (child_exited)
	    {
		y2warning ("looks like child exited...");
		return ret;
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
 * Write - nothing to do
 */
YCPBoolean ThinkFingerAgent::Write(const YCPPath &path, const YCPValue& value,
	                             const YCPValue& arg)
{
    return YCPBoolean(false);
}

/**
 * Execute(.thinkfinger.add-user) is action to acquire fingerprint
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
	    child_pid	= -1;
	    if (tf) libthinkfinger_free (tf);
	    ret = YCPBoolean (true);
	}
	/**
	 * parameter is whole path to target bir file, e.g.
	 * /tmp/YaST-123-456/hh.bir
	 */
	else if (PC(0) == "add-user") {
	    string bir_path;
	    if (!val.isNull())
	    {
		bir_path = val->asString()->value();
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

	    libthinkfinger_init_status init_status;
	    tf = libthinkfinger_new (&init_status);
	    if (init_status != TF_INIT_SUCCESS) {
		y2error ("libthinkfinger_new failed");
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
	    signal (SIGCHLD, catch_child_exit);
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

y2internal ("path is '%s'", bir_path.c_str());

		if (libthinkfinger_set_file (tf, bir_path.c_str ()) < 0)
		{
		    y2error ("libthinkfinger_set_file failed");
		    if (tf) libthinkfinger_free (tf);
		    retval		= SET_FILE_FAILED;
		    write (data_pipe[1], &retval, sizeof(libthinkfinger_state));
		    close (data_pipe[1]);
		    exit (retval);		
		}
		if (libthinkfinger_set_callback (tf, callback, &tfdata) < 0)
		{
		    y2error ("libthinkfinger_set_callback failed");
		    if (tf) libthinkfinger_free (tf);
		    retval		= SET_CALLBACK_FAILED;
		    write (data_pipe[1], &retval, sizeof(libthinkfinger_state));
		    close (data_pipe[1]);
		    exit (retval);		
		}
		int tf_state = libthinkfinger_acquire (tf);
		y2milestone ("acquire done with state %d", tf_state);
		close (data_pipe[1]);
		if (tf) libthinkfinger_free (tf);
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

