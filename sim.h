#ifndef _SIM_H_
#define _SIM_H_

#include "util.h"

// Represents the different pipeline datapaths and their execution.

// Constants indicating which stage performs specified functionality
#define PIPE_ENTER_STAGE FETCH

// Stalling pipeline
#define PIPE_WRITE_REG_STAGE WRITEBACK
#define PIPE_READ_REG_STAGE DECODE

/*
// Data forwarding pipeline
#define PIPE_READ_FORWARD_REG_STAGE EXECUTE
#define PIPE_ALU_PRODUCE_STAGE EXECUTE
#define PIPE_MEMORY_PRODUCE_STAGE MEMORY

// For control hazards
#define PIPE_KNOW_NEXTPC EXECUTE

// Pipeline stages.  NUM_STAGES is not an actual stage but represents the
// number of stages.
typedef enum {FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK, NUM_STAGES} PipeStage;
*/

#define PIPE_READ_FORWARD_REG_STAGE EXECUTE1
#define PIPE_ALU_PRODUCE_STAGE EXECUTE2
#define PIPE_MEMORY_PRODUCE_STAGE MEMORY2
#define PIPE_KNOW_NEXTPC EXECUTE2
typedef enum {FETCH, DECODE, EXECUTE1, EXECUTE2, MEMORY1, MEMORY2, WRITEBACK, NUM_STAGES} PipeStage;

// For each Instruction, track when it exits the pipeline
typedef struct {
  Instruction *instr;
  int completionTime;
} InstructionCompletion;

// Given an array of instructions, execute the specified number of
// instructions on the pipeline and return an array of InstructionCompletion
// instances, one for each of the numInstrs that indicates the instruction's
// completion time.  This pipeline is ideal in that no instructions stall.
InstructionCompletion *executeNoHazardPipeline(Instruction *instrs,
					       int numInstrs);

// Given an array of instructions, execute the specified number of
// instructions on the pipeline and return an array of InstructionCompletion
// instances, one for each of the numInstrs that indicates the instruction's
// completion time.  This pipeline stalls instructions waiting for their
// source operand registers to be written by an instruction already in the
// pipeline until that instruction has been to the register file.  It also
// stalls on control instructions until the target destination is know.
InstructionCompletion * executeStallPipeline(Instruction *instrs,
					     int numInstrs);


// given the current state of the Pipeline and the stage it is on
// this will check if there are any uses of registers in the
// execute and memory stage that need to be written back before accessing in
// the decode stage
int stallOrNot(Instruction *instrs[NUM_STAGES], int stages);

int controlHazard(Instruction *instrs[NUM_STAGES]);

// Given an array of instructions, execute the specified number of
// instructions on the pipeline and return an array of
// InstructionCompletion instances, one for each of the numInstrs that
// indicates the instruction's completion time.  This pipeline stalls
// instructions waiting for their source operand registers to be
// written by an instruction already in the pipeline until that
// instruction has produced the value and can forward it to the
// instruction needing it's value.  It also stalls on control
// instructions until the target destination is know.
InstructionCompletion * executeForwardPipeline(Instruction *instrs,
					       int numInstrs);
						   
// given the current state of the Pipeline and the stage it is on
// this will check for need of stalling in data forwarding pipeline
int stallOrNotII(Instruction *pipeline[NUM_STAGES], int stage);

// Returns 1 if all pipeline stages are empty.  Returns 0 otherwise.
int isPipelineEmpty(Instruction *pipeline[]);

// For a specified pipeline, print out the specified number of instructions
// and their completion times to stdout
void printCompletionTimes(char *pipelineName,
			  InstructionCompletion *instrTimes, int numInstrs);

#endif
