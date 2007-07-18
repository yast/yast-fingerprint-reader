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

// for testing only...
#define MAX_PATH       256
#define BIR_EXTENSION    ".bir"
#define PAM_BIRDIR "/etc/pam_thinkfinger"

/**
 * @short An interface class between YaST2 and ThinkFinger Agent
 */
class ThinkFingerAgent : public SCRAgent
{
private:

    /**
     * pid of the child process after fork
     */
    pid_t child_pid;

public:
    /**
     * Default constructor.
     */
    ThinkFingerAgent();

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
};

#endif /* _ThinkFingerAgent_h */
