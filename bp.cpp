/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <limits>

enum FSM : unsigned {SNT=0, WNT=1, WT=2, ST=3};

FSM incrFSM(FSM prev) {
    if (prev == ST) return ST;
    return static_cast<FSM>(prev + 1);
}

FSM decrFSM(FSM prev) {
    if (prev == SNT) return SNT;
    return static_cast<FSM>(prev - 1);
}

enum ShareType : int {not_using_share=0, using_share_lsb=1, using_share_mid=2};

// log base 2 of an unsigned integer, rounded down
// note: there are assembly instructions to do this but inline assembly sucks...
inline unsigned log2(unsigned x) {
    unsigned l = 0;
    if (x == 0) return std::numeric_limits<unsigned>::max();
    while(x >>= 1) ++l;
    return l;
}

// returns 2^x
inline uint32_t twopow(uint32_t x) {
    return 1 << x;
}

// create a bitmask where the last x bits are 1
// for example: lsbmask(5) produces 00...00011111 in binary
inline uint32_t lsbmask(uint32_t x) {
    return (1 << x) - 1;
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
    ShareType sharedType;

    // vectors of tags and valid bit
    std::vector<bool> valid;
    std::vector<uint32_t> tags;
    std::vector<uint32_t> targets;

    std::vector<uint8_t> histories;
    std::vector<std::vector<FSM>> fsms;

    // private struct to hold the separated parts of the program counter
    struct PCParts
    {
        uint8_t list_index;
        uint32_t tag;
        uint8_t share_bits;
    };

    // Split a given program counter into its different bitwise parts
    PCParts SplitPC(uint32_t pc) {
        PCParts parts;

        // first get the share bits, before modifying pc
        if (isGlobalHist) {
            switch(sharedType) {
                case not_using_share:
                    parts.share_bits = 0;
                    break;
                case using_share_lsb:
                    parts.share_bits = (pc >> 2) | lsbmask(historySize);
                    break;
                case using_share_mid:
                    parts.share_bits = (pc >> 16) | lsbmask(historySize);
                    break;
            }
        } else {
            parts.share_bits = 0;
        }

        // ignore first 2 bits (4-byte aligned)
        pc >>= 2;

        // next log2(btbSize) bits are the list index
        uint32_t li_length = log2(btbSize);
        parts.list_index = pc & lsbmask(li_length);
        pc >>= li_length;

        // next tagSize bits are the tag
        parts.tag = pc & lsbmask(tagSize);

        return parts;
    }

public:
    BranchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
                    bool isGlobalHist, bool isGlobalTable, int isShared) :

                    // initialize parameters
                    btbSize(btbSize), historySize(historySize), tagSize(tagSize), fsmState(fsmState),
                    isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), sharedType(static_cast<ShareType>(isShared)),

                    // initialize tags and valid vectors
                    valid(btbSize, 0), tags(btbSize), targets(btbSize),

                    // initialize histories vector -- if global history, just one element,
                    // otherwise it's the size of the BTB
                    histories(isGlobalHist ? 1 : btbSize),

                    // initialize FSM (bimodal) vector -- each element contains a vector of
                    // one FSM per history
                    // again, if the table is global, there will be just one set of FSMs
                    fsms(isGlobalTable ? 1 : btbSize, std::vector<FSM>(twopow(historySize), static_cast<FSM>(fsmState)))
    {
        // initialize the stats
        stats.flush_num = 0;
        stats.br_num = 0;

        // calculate the predictor size
        stats.size = 1 * valid.size()           // valid bits
            + tagSize * tags.size()             // tag bits
            + 30 * targets.size()               // targets bits (30 bits for target address, lower 2 bits assumed to be 0)
            + historySize * histories.size()    // history bits
            + 2 * fsms.size() * fsms[0].size(); // FSM bits


    }

    bool predict(uint32_t pc, uint32_t *dst) {
        PCParts parts = SplitPC(pc);
        uint8_t i = parts.list_index; // convenience handle

        // first of all - do we even have this PC in our table?
        if (!valid[i] || tags[i] != parts.tag) {
            *dst = pc + 4;
            return false;
        }

        // get our history
        uint8_t hist = histories[isGlobalHist ? 0 : i];
        // XOR it with LShare/GShare
        hist ^= parts.share_bits;

        // use it as index for the FSM
        FSM state = fsms[isGlobalTable ? 0 : i][hist];

        // use the state of the FSM to predict outcome
        if (state == WT || state == ST) {
            *dst = targets[i];
            return true;
        } else {
            *dst = pc + 4;
            return false;
        }
    }

    void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
        PCParts parts = SplitPC(pc);
        uint8_t i = parts.list_index; // convenience handle

        // Update stats
        ++stats.br_num;
        bool prediction_was_wrong = (!taken && pred_dst != pc+4) || (taken && pred_dst != targetPc);
        if (prediction_was_wrong) ++stats.flush_num;

        // find out if this is an existing entry or new/replacing
        bool isOld = valid[i] && (tags[i] == parts.tag);

        // if it's old, initialize the entry in the table
        if (!isOld) {
            valid[i] = true;
            tags[i] = parts.tag;
            if (!isGlobalHist) histories[i] = 0;
            if (!isGlobalTable) fsms[i].assign(twopow(historySize), static_cast<FSM>(fsmState));
        }

        // update target address (even for old entries it might change)
        targets[i] = targetPc;

        // update fsm
        uint8_t fsm_index = isGlobalTable ? 0 : i;
        uint8_t hist_index = isGlobalHist ? 0 : i;
        uint8_t hist = histories[hist_index];
        uint8_t fsm_sub_index = hist ^ parts.share_bits;
        if (taken) {
            fsms[fsm_index][fsm_sub_index] = incrFSM(fsms[fsm_index][fsm_sub_index]);
        } else {
            fsms[fsm_index][fsm_sub_index] = decrFSM(fsms[fsm_index][fsm_sub_index]);
        }

        // update history
        histories[hist_index] = ((hist << 1) | taken) & lsbmask(historySize);
    }

    void GetStats(SIM_stats *curStats) {
        *curStats = stats;
    }
};

BranchPredictor *pred;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int isShared){
    try {
        pred = new BranchPredictor(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, isShared);
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

