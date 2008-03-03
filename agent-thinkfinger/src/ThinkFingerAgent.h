/* ThinkFingerAgent.h
 *
 * ThinkFinger agent implementation
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id: ThinkFingerAgent.h 26456 2005-12-07 16:11:23Z jsuchome $
 */

#ifndef _ThinkFingerAgent_h
#define _ThinkFingerAgent_h

#include <Y2.h>
#include <scr/SCRAgent.h>

using namespace std;

#include <libthinkfinger.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string>

/**
 * The real interface to thinkfinger library
 * - extra (singleton) class is used to contain signal handler function
 */
class ThinkFingerAPI
{

private:
    ThinkFingerAPI ();
	        
    virtual ~ThinkFingerAPI();

public:

    static ThinkFingerAPI & instance();

    static void catch_sigterm (int);

    int acquire (int, string);

    int test_int;

    libthinkfinger *tf;

    void finalize ();

};

/**
 * @short An interface class between YaST2 and ThinkFinger Agent
 */
class ThinkFingerAgent : public SCRAgent
{

public:
    
    /**
     * Default constructor.
     */
    ThinkFingerAgent ();
	        

    /**
     * Destructor.
     */
    virtual ~ThinkFingerAgent();

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

#endif /* _ThinkFingerAgent_h */
