/* Y2CCThinkFingerAgent.cc
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 * $Id$
 */

#include <scr/Y2AgentComponent.h>
#include <scr/Y2CCAgentComponent.h>

#include "ThinkFingerAgent.h"

typedef Y2AgentComp <ThinkFingerAgent> Y2ThinkFingerAgentComp;

Y2CCAgentComp <Y2ThinkFingerAgentComp> g_y2ccag_thinkfinger ("ag_thinkfinger");
