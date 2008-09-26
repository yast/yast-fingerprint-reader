/* Y2CCFPrintAgent.cc
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 */

#include <scr/Y2AgentComponent.h>
#include <scr/Y2CCAgentComponent.h>

#include "FPrintAgent.h"

typedef Y2AgentComp <FPrintAgent> Y2FPrintAgentComp;

Y2CCAgentComp <Y2FPrintAgentComp> g_y2ccag_fprint ("ag_fprint");
