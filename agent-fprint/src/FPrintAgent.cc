/* FPrintAgent.cc
 *
 * An agent for FPrint library (access to fingerprint reader)
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id$
 */

#include <unistd.h>

#include "FPrintAgent.h"

#define PC(n)       (path->component_str(n))

// error exit values of the child process
#define INIT_FAILED		200
#define NO_DEVICES		201
#define DEVICE_OPEN_FAILED	202
#define ENROLL_FAILURE		255


/**
 * Constructor
 */
FPrintAgent::FPrintAgent() : SCRAgent()
{
    child_pid		= -1;
    child_retval	= -1;
}

/**
 * Destructor
 */
FPrintAgent::~FPrintAgent()
{
}

// return the only instance of the class
FPrintAPI& FPrintAPI::instance()
{
    static FPrintAPI _instance; // The singleton
    return _instance;
}

// handler for SIGTERM signal (it is necessary to kill the process when
// user his Cancel)
void FPrintAPI::catch_sigterm (int sig_num)
{
    instance().finalize ();
    _exit (256);
}

// de-initialize finger print reader (must be called at the end!)
void FPrintAPI::finalize ()
{
    if (instance().data != NULL)
    {
	fp_print_data_free (instance().data);
	instance().data	= NULL;
    }
    fp_exit();
}

FPrintAPI::FPrintAPI()
{
    data	= NULL;
}

FPrintAPI::~FPrintAPI ()
{
}

/**
 * wrapper for fp_enroll_finger function;
 * - we need to take care of the errors and signals
 * @param file descriptor of the pipe  
 * @param path to the temporary directory for storing fingerprints
 */
int FPrintAPI::acquire (int write_fd, string dir_path)
{
    // catch the abort signal to make proper cleanup
    signal (15, catch_sigterm);

    struct fp_print_data *enrolled_print = NULL;
    int r;

    struct fp_dscv_dev *ddev;
    struct fp_dscv_dev **discovered_devs;
    struct fp_dev *dev;

    r = fp_init();
    if (r < 0) {
	y2error("Failed to initialize libfprint");
	r	= INIT_FAILED;
	write (write_fd, &r, sizeof(int));
	return r;
    }
    
    discovered_devs = fp_discover_devs();
    if (!discovered_devs) {
	y2error("Could not discover devices");
	instance().finalize ();
	r	= NO_DEVICES;
	write (write_fd, &r, sizeof(int));
	return r;
    }

    ddev = discovered_devs[0];

    if (!ddev) {
	y2error("No devices detected.");
	instance().finalize ();
	r	= NO_DEVICES;
	write (write_fd, &r, sizeof(int));
	return r;
    }
	
	/*
	struct fp_driver *drv;
	drv = fp_dscv_dev_get_driver(ddev);
	y2milestone ("Found device claimed by %s driver",
	    fp_driver_get_full_name(drv));
	*/

    dev = fp_dev_open(ddev);
    fp_dscv_devs_free(discovered_devs);
    if (!dev) {
	y2error("Could not open device.");
	instance().finalize ();
	r	= DEVICE_OPEN_FAILED;
	write (write_fd, &r, sizeof(int));
	return r;
    }

    y2milestone ("Device opened successfully.");

    /* TODO how to pass the info about stages number to the parent?
    y2milestone ("You will need to successfully scan your finger %d times to "
	"complete the process.", fp_dev_get_nr_enroll_stages (dev));
    */ 

    do {
	sleep (1);

	r = fp_enroll_finger (dev, &enrolled_print);

	y2debug ("retval: %d", r);

	if (r < 0) {
	    y2error ("Enroll failed with error %d", r);
	    instance().finalize ();
	    signal (15, SIG_DFL);
	    return ENROLL_FAILURE;
	}

	if (write (write_fd, &r, sizeof (int)) == -1)
	    y2error ("write to pipe failed: %d (%m)", errno);


	switch (r) {
		case FP_ENROLL_COMPLETE:
			y2milestone ("Enroll complete!");
			break;
		case FP_ENROLL_FAIL:
			y2error ("Enroll failed, something wen't wrong :(");
			instance().finalize ();
			signal (15, SIG_DFL);
			return ENROLL_FAILURE;
		case FP_ENROLL_PASS:
			break;
		case FP_ENROLL_RETRY:
			break;
		case FP_ENROLL_RETRY_TOO_SHORT:
			break;
		case FP_ENROLL_RETRY_CENTER_FINGER:
			break;
		case FP_ENROLL_RETRY_REMOVE_FINGER:
			break;
	}
    } while (r != FP_ENROLL_COMPLETE);

    if (!enrolled_print) {
	y2error ("Enroll complete but print is null");
	r	= ENROLL_FAILURE;
    }
    else {
	// set the HOME to temporary path we know from argument
	setenv ("HOME", dir_path.c_str(), 1);
	data	= enrolled_print;
	int save_r = fp_print_data_save (data, RIGHT_INDEX);
	if (save_r < 0)
	{
	    y2error ("Data save failed with %d", r);
	    r	= save_r;
	}
	y2milestone ("Enrollment completed, exiting with %d", r);
    }

    instance().finalize ();
    signal (15, SIG_DFL);
    return r;
}

/**
 * Dir
 */
YCPList FPrintAgent::Dir(const YCPPath& path) 
{   
    y2error("Wrong path '%s' in Dir().", path->toString().c_str());
    return YCPNull();
}

/**
 * Read
 */
YCPValue FPrintAgent::Read(const YCPPath &path, const YCPValue& arg, const YCPValue& opt) {

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
		y2error ("FPrint not initialized yet!");
		return ret;
	    }
	    int state;
	    size_t size	= sizeof (int);
	    int retval = read (data_pipe[0], &state, size);
	    if (retval == -1)
	    {
		if (errno != EINTR && errno != EAGAIN)
		    y2error ("error reading from pipe: %d (%m)", errno);
	    }
	    else if (retval == size) {
		if (state == INIT_FAILED ||
		    state == NO_DEVICES || state == DEVICE_OPEN_FAILED)
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
	// check if child process exited
	else if (PC(0) == "check_exit" ) {
	    int status;
	    ret = YCPBoolean (false);
	    if (child_pid != -1)
	    {
		if (waitpid (child_pid, &status, WNOHANG) != 0)
		{
		    child_pid	= -1;
		    ret	= YCPBoolean (true);
		    if (WIFSIGNALED (status))
		    {
			y2error ("child process was killed");
		    }
		    if (WIFEXITED (status))
		    {
			child_retval	= WEXITSTATUS (status);
		    }
		}
	    }
	    else
	    {
		y2milestone ("child process already exited");
		ret	= YCPBoolean (true);
	    }
	}
	// wait for child exit if it still not exited (do not use this part)
	// return child exit value
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
		y2milestone ("child process already exited");
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
YCPBoolean FPrintAgent::Write(const YCPPath &path, const YCPValue& value,
	                             const YCPValue& arg)
{
    return YCPBoolean(false);
}

/**
 * Execute(.fprint.enroll) is action to acquire fingerprint
 */
YCPValue FPrintAgent::Execute(const YCPPath &path, const YCPValue& val, const YCPValue& arg)
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
			y2milestone ("... still alive, killing it");
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
	else if (PC(0) == "enroll") {
	    string path;
	    if (!val.isNull())
	    {
		path = val->asString()->value();
	    }
	    else
	    {
		y2error ("path to tmp directory is missing");
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
		    FPrintAPI::instance().acquire (data_pipe[1], path);
		y2milestone ("acquire done with state %d", state);
		close (data_pipe[1]);
		_exit (state);
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
YCPValue FPrintAgent::otherCommand(const YCPTerm& term)
{
    string sym = term->name();

    if (sym == "FPrintAgent") {
        
        return YCPVoid();
    }

    return YCPNull();
}

