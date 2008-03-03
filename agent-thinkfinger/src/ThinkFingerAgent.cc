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
    child_retval	= -1;
}

/**
 * Destructor
 */
ThinkFingerAgent::~ThinkFingerAgent()
{
}

// return the only instance of the class
ThinkFingerAPI& ThinkFingerAPI::instance()
{
    static ThinkFingerAPI _instance; // The singleton
    return _instance;
}

// handler for SIGTERM signal (it is necessary to kill the process when
// user his Cancel)
void ThinkFingerAPI::catch_sigterm (int sig_num)
{
    instance().finalize ();
    exit (256);
}

// de-initialize finger print reader (must be called at the end!)
void ThinkFingerAPI::finalize ()
{
    if (instance().tf != NULL)
    {
	libthinkfinger_free (instance().tf);
	instance().tf	= NULL;
    }
}

ThinkFingerAPI::ThinkFingerAPI()
{
    tf			= NULL;
}

ThinkFingerAPI::~ThinkFingerAPI ()
{
}

/**
 * wrapper for libthinkfinger_acquire function;
 * - we need to take care of the errors and signals
 * @param file descriptor of the pipe
 * @param path to the target bir file
 */
int ThinkFingerAPI::acquire (int write_fd, string bir_path)
{
    signal (15, catch_sigterm);

    int retval	= 255;
    s_tfdata tfdata;
    tfdata.write_fd		= write_fd;

    libthinkfinger_init_status init_status;
    instance().tf = libthinkfinger_new (&init_status);
    if (init_status != TF_INIT_SUCCESS) {
	y2error ("libthinkfinger_new failed");
	instance().finalize ();
	retval	= INIT_FAILED;
	write (write_fd, &retval, sizeof(libthinkfinger_state));
	return retval;
    }

    if (libthinkfinger_set_file (instance().tf, bir_path.c_str ()) < 0)
    {
	y2error ("libthinkfinger_set_file failed");
	instance().finalize ();
	retval	= SET_FILE_FAILED;
	write (write_fd, &retval, sizeof(libthinkfinger_state));
	return retval;		
    }
    if (libthinkfinger_set_callback (instance().tf, callback, &tfdata) < 0)
    {
	y2error ("libthinkfinger_set_callback failed");
	instance().finalize ();
	retval	= SET_CALLBACK_FAILED;
	write (write_fd, &retval, sizeof(libthinkfinger_state));
	return retval;		
    }
    retval = libthinkfinger_acquire (ThinkFingerAPI::instance().tf);
    y2milestone ("acquire done with state %d", retval);
    instance().finalize ();
    signal (15, SIG_DFL);
    return retval;
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
	    YCPMap retmap;
	    if (child_pid == -1)
	    {
		y2error ("ThinkFinger not initialized yet!");
		return ret;
	    }
	    int state;
	    size_t size	= sizeof (libthinkfinger_state);
	    int retval = read (data_pipe[0], &state, size);
	    if (retval == -1)
	    {
		if (errno != EINTR && errno != EAGAIN)
		    y2error ("error reading from pipe: %d (%m)", errno);
	    }
	    else if (retval == size) {
		if (state == INIT_FAILED ||
		    state == SET_FILE_FAILED || state == SET_CALLBACK_FAILED)
		{
		    y2warning ("some initialization failed (%d)...", state);
		    return ret;
		}
		retmap->add (YCPString ("state"), YCPInteger (state));
	    }
	    else if (retval == 0)
	    {
		// check if child is still alive
		int status;
		if (waitpid (-1, &status, WNOHANG) != 0)
		{
		    if (WIFSIGNALED (status))
		    {
			y2error ("child process was killed");
			child_pid	= -1;
			return ret;
		    }
		    if (WIFEXITED (status))
		    {
			child_retval	= WEXITSTATUS (status);
			child_pid	= -1;
			return ret;
		    }
		}
	    }
	    return retmap;
	}
	// wait for child exit
	else if (PC(0) == "exit_status" ) {
	    int status;
	    int retval	= 255;
	    if (child_pid != -1)
	    {
		wait (&status);
		if (WIFSIGNALED (status))
		    y2milestone ("child process was killed");
		else if (WIFEXITED (status))
		{
		    retval	= WEXITSTATUS (status);
		    y2milestone ("retval is %d", retval);
		}
	    }
	    else
	    {
		y2milestone ("child process already dead");
		if (child_retval)
		    retval	= child_retval;
		child_retval	= -1;
	    }
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
	    y2milestone ("terminanting child process with pid %d", child_pid);
	    if (child_pid)
	    {
		if (kill (child_pid, 15) != -1)
		{
		    sleep (3);
		    int status;
		    if (waitpid (-1, &status, WNOHANG) == 0)
		    {
			y2milestone ("... still alive, killing it", child_pid);
			kill (child_pid, 9);
			wait (&status);
		    }
		    child_pid	= -1;
		}
		else
		{
		    y2error ("error while killing: %d (%m)", errno);
		}
	    }
	    ret = YCPBoolean (true);
	}
	/**
	 * parameter is whole path to target bir file, e.g.
	 * /tmp/YaST-123-456/hh.bir
	 */
	else if (PC(0) == "add-user") {
	    string path;
	    if (!val.isNull())
	    {
		path = val->asString()->value();
	    }
	    else
	    {
		y2error ("path to bir file is missing");
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
		close (data_pipe[0]); // close the read-only FD
		int state =
		    ThinkFingerAPI::instance().acquire (data_pipe[1], path);
		y2milestone ("acquire done with state %d", state);
		close (data_pipe[1]);
		exit (state);
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

