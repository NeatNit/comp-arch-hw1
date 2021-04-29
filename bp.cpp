/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

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


public:
    BranchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
                    bool isGlobalHist, bool isGlobalTable, int isShared) :
                    btbSize(btbSize), historySize(historySize), tagSize(tagSize), fsmState(fsmState),
                    isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), isShared(Shared)
    {
        stats.flush_num = 0;
        stats.br_num = 0;
        stats.size = 0; // actually we can calculate size here


    }
    ~BranchPredictor();

    bool predict(uint32_t pc, uint32_t *dst) {

    }

    void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {

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
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

