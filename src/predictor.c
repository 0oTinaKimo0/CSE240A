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
const char *studentName = "Tina Jin";
const char *studentID   = "A14463292";
const char *email       = "dojin@ucsd.edu";

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
uint8_t *pht;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  gsize = 1 << ghistoryBits;
  lsb = gsize - 1;
  
  switch (bpType) {
    case GSHARE:
      return init_gshare();
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc) {
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return pred_gshare(pc);
    case TOURNAMENT:
    case CUSTOM:
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
      return train_gshare(pc, outcome);
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
}

// Helper functions to be called for each type of predictor
void init_gshare() {
  ghr = 0;
  pht = (uint8_t *) malloc(sizeof(uint8_t) * gsize);
  memset(pht, 1, gsize); // all entries in the pht are initialized to 01 (WN)
}

void pred_gshare(uint32_t pc) {
  uint32_t index = (ghr ^ pc) & lsb; // global history register is xored with the PC to index into pht
  return pht[index] >> 1;
}

void train_gshare(uint32_t pc, uint8_t outcome) {
  ghr = (ghr << 1 | outcome) & lsb; // update global history register to be the new outcome 
  uint32_t index = (ghr ^ pc) & lsb;
  // update pht by incrementing or decrementing the 2-bit counter
  uint8_t currP = pht[index];
  if (outcome) {
    if (currP != 3)
      pht[index] += 1;
  } else {
    if (currP != 0)
      pht[index] -= 1;
  }
}