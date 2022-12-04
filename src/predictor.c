//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Ryan Dong";
const char *studentID   = "A59018151";
const char *email       = "rudong@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

// global
uint32_t gsize;
uint32_t lsb;
// gshare
uint32_t ghr;
uint8_t* pht;
// tournament
uint32_t* lhr;            // local history register
uint32_t lhrSize;       // size of local history register
uint32_t* lpht;            // local pattern history table
uint32_t lphtSize;      // size of local pattern history table
uint32_t* cpht;          // choice pattern history table
// custom - yeh-patt
// uint8_t* ypht;

void init_gshare();
uint8_t pred_gshare(uint32_t pc);
void train_gshare(uint32_t pc, uint8_t outcome);

void init_tournament();
uint8_t pred_tournament(uint32_t pc);
void train_tournament(uint32_t pc, uint8_t outcome);

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void init_predictor() {
  gsize = 1 << ghistoryBits;
  lsb = gsize - 1;

  switch (bpType) {
    case GSHARE:
      init_gshare();
    case TOURNAMENT:
      init_tournament();
    //case CUSTOM:
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc) {
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return pred_gshare(pc);
    case TOURNAMENT:
      return pred_tournament(pc);
    //case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint8_t outcome) {
  switch (bpType) {
    case GSHARE:
      train_gshare(pc, outcome);
    case TOURNAMENT:
      train_tournament(pc, outcome);
    //case CUSTOM:
    default:
      break;
  }
}

// Helper functions to be called for each type of predictor
// gshare
void init_gshare() {
  ghr = 0;
  pht = (uint8_t *) malloc(sizeof(uint8_t) * gsize);
  for (int i = 0; i < gsize; i++) {
    pht[i] = WN; // all entries in the pht are initialized to 01 (WN)
  }
}

uint8_t pred_gshare(uint32_t pc) {
  uint32_t index = (ghr ^ pc) & lsb; // global history register is xored with the PC to index into pht
  return pht[index] >> 1;
}

void train_gshare(uint32_t pc, uint8_t outcome) {
  ghr = (ghr << 1 | outcome) & lsb; // update global history register to be the new outcome
  uint32_t index = (ghr ^ pc) & lsb;
  // update pht by incrementing or decrementing the 2-bit counter
  uint8_t currP = pht[index];
  if (outcome) {
    if (pht[index] != 3) pht[ghr]++;
  }
  else {
    if (pht[index] != 0) pht[ghr]--;
  }
}

// tournament
void init_tournament() {
    init_gshare();
    lhrSize = 1 << pcIndexBits;
    lhr = (uint32_t*)malloc(sizeof(uint32_t) * lhrSize);
    for (int i = 0; i < lhrSize; i++) {
        lhr[i] = NOTTAKEN;
    }
    lphtSize = 1 << lhistoryBits;
    lpht = (uint32_t*)malloc(sizeof(uint32_t) * lphtSize);
    for (int i = 0; i < lphtSize; i++) {
        lpht[i] = WN;
    }
    cpht = (uint32_t*)malloc(sizeof(uint32_t) * gsize);
    for (int i = 0; i < gsize; i++) {
        cpht[i] = WN;
    }
}

uint8_t pred_local(uint32_t pc) {
    uint32_t  LSB = pc & (lhrSize - 1);  // least significant bits of pc
    uint32_t branchHistory = lhr[LSB];
    if (lpht[branchHistory] >= WT) return TAKEN;
    else  return NOTTAKEN;
}

uint8_t pred_global(uint32_t pc) {
    uint32_t phtIndex = ghr & (gsize - 1);  // least significant bits of pc
    if (pht[phtIndex] >= WT)  return TAKEN;
    else  return NOTTAKEN;
}

uint8_t pred_tournament(uint32_t pc) {
    uint32_t predSelection = cpht[ghr];
    if (predSelection <= WN) {
        return pred_global(pc);
    }
    else {
        return pred_local(pc);
    }
}

void train_tournament(uint32_t pc, uint8_t outcome) {
    // choice selector
    uint32_t globalOutcome = pred_global(pc);
    uint32_t localOutcome = pred_local(pc);
    if (globalOutcome != localOutcome) {
        if (globalOutcome == outcome && cpht[ghr] != 0) {
            cpht[ghr]--;
        }
        if (localOutcome == outcome && cpht[ghr] != 3) {
            cpht[ghr]++;
        }
    }
    // gshare predictor
    if (outcome) {
        if (pht[ghr] != 3) pht[ghr]++;
    }
    else {
        if (pht[ghr] != 0) pht[ghr]--;
    }
    // local predictor
    uint32_t LSB = pc & (lhrSize - 1);  // least significant bits of pc
    uint32_t lphtIndex = lhr[LSB];
    if (outcome) {
        if (lpht[lphtIndex] != 3) lpht[lphtIndex]++;
    }
    else {
        if (lpht[lphtIndex] != 0) lpht[lphtIndex]--;
    }
    ghr = (ghr << 1 | outcome) & lsb;
    lhr[LSB] = (lhr[LSB] << 1 | outcome) & (lphtSize - 1);
}
