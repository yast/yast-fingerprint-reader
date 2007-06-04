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

#include <iostream>
#include <libthinkfinger.h>

#include <string>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libintl.h>
#include <fstream>
#include <pthread.h>

// for testing only...
//#define DEFAULT_BIR_PATH "/tmp/test.bir"
#define MAX_PATH       256
// #define MODE_ACQUIRE   1
#define BIR_EXTENSION    ".bir"
#define PAM_BIRDIR "/etc/pam_thinkfinger"

/**
 * @short An interface class between YaST2 and ThinkFinger Agent
 */
class ThinkFingerAgent : public SCRAgent
{
private:

    libthinkfinger *tf;

    pthread_t pt;

    string user;
    /**
     * Forbid any calls, when not initialized
     */
    bool initialized;

    /*
    libthinkfinger_state global_state; // save the last state

    s_tfdata swipes; // save the nubmer of (un)succ. swipes globally
    */

    /**
     * Call ThinkFinger->Recover in the new thread
     */
//    static void *call_recover (ThinkFingerAgent *);

    static void *call_acquire (ThinkFingerAgent *);

//    static void *call_acquire (libthinkfinger *my_tf);

//    void callback (libthinkfinger_state, void *);

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

    libthinkfinger_state global_state; // save the last state

    int swipe_success;
    int swipe_failed; // FIXME use functions!
//    s_tfdata swipes; // save the nubmer of (un)succ. swipes globally
};

#endif /* _ThinkFingerAgent_h */
