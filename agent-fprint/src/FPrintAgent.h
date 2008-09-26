/* FPrintAgent.h
 *
 * FPrint agent implementation
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id: FPrintAgent.h 26456 2005-12-07 16:11:23Z jsuchome $
 */

#ifndef _FPrintAgent_h
#define _FPrintAgent_h

#include <Y2.h>
#include <scr/SCRAgent.h>

using namespace std;

#include <libfprint/fprint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string>

/**
 * The real interface to fprint library
 * - extra (singleton) class is used to contain signal handler function
 */
class FPrintAPI
{

private:
    FPrintAPI ();
	        
    virtual ~FPrintAPI();

public:

    static FPrintAPI & instance();

    static void catch_sigterm (int);

    int acquire (int, string);

    int test_int;

    struct fp_print_data *data;

    void finalize ();

};

/**
 * @short An interface class between YaST2 and FPrint Agent
 */
class FPrintAgent : public SCRAgent
{

public:
    
    /**
     * Default constructor.
     */
    FPrintAgent ();
	        

    /**
     * Destructor.
     */
    virtual ~FPrintAgent();

    /**
     * Provides SCR Read ().
     * @param path Path that should be read.
     * @param arg Additional parameter.
     */
    virtual YCPValue Read(const YCPPath &path,
			  const YCPValue& arg = YCPNull(),
			  const YCPValue& opt = YCPNull());

    /**
     * Provides SCR Write ().
     */
    virtual YCPBoolean Write(const YCPPath &path,
			   const YCPValue& val,
			   const YCPValue& arg = YCPNull());

    /**
     * Provides SCR Execute ().
     */
    virtual YCPValue Execute(const YCPPath &path,
			     const YCPValue& val = YCPNull(),
			     const YCPValue& arg = YCPNull());
                 
    /**
     * Provides SCR Dir ().
     */
    virtual YCPList Dir(const YCPPath& path);

    /**
     * Used for mounting the agent.
     */
    virtual YCPValue otherCommand(const YCPTerm& term);

    /**
     * array with pipe file descriptors
     */
    int data_pipe[2];

    /**
     * pid of the child process after fork
     */
    pid_t child_pid;

    /**
     * return value of the child process
     */
    int child_retval;

};

#endif /* _FPrintAgent_h */
