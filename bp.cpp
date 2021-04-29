/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <limits>

enum FSM : unsigned char {SNT=0, WNT=1, WT=2, ST=3};

// log base 2 of an unsigned integer, rounded down
// note: there are assembly instructions to do this but inline assembly sucks...
unsigned log2(unsigned x) {
    unsigned l = 0;
    if (x == 0) return std::numeric_limits<unsigned>::max();
    while(x >>= 1) ++l;
    return l;
}

class BranchPredictor
{
    SIM_stats stats;
    unsigned btbSize;
    unsigned historySize;
    unsigned tagSize;
    unsigned fsmState;
    bool isGlobalHist;
    bool isGlobalTable;
    int Shared;

    // vectors of tags and valid bit
    std::vector<unsigned> tag;
    std::vector<bool> valid;

    std::vector<unsigned char> history;
    std::vector<std::vector<FSM>> fsm;
public:
    BranchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
                    bool isGlobalHist, bool isGlobalTable, int isShared) :

                    // initialize parameters
                    btbSize(btbSize), historySize(historySize), tagSize(tagSize), fsmState(fsmState),
                    isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), isShared(Shared),

                    // initialize tag and valid vectors
                    tag(btbSize), valid(btbSize, 0),

                    // initialize history vector -- if global history, just one element,
                    // otherwise it's the size of the BTB
                    history(isGlobalHist ? 1 : btbSize),

                    // initialize FSM (bimodal) vector -- each element contains a vector of
                    // one FSMs per history
                    // again, if the table is global, there will be just one set of FSMs
                    fsm(isGlobalTable ? 1 : btbSize, std::vector<FSM>(log2(historySize), fsmState))
    {
        stats.flush_num = 0;
        stats.br_num = 0;
        stats.size = 0; // actually we can calculate size here


    }
    ~BranchPredictor();

    bool predict(uint32_t pc, uint32_t *dst) {
        return false;
    }

    void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
        return;
    }

    void GetStats(SIM_stats *curStats) {
        *curStats = stats;
    }
};

BranchPredictor *pred;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
    try {
        *pred = new BranchPredictor(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
        return 0;
    } catch (...) {
        return -1;
    }
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return pred->predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	pred->update(pc, targetPc, taken, pred_dst);
}

void BP_GetStats(SIM_stats *curStats){
	pred->GetStats(curStats);
    delete pred;
}

