#include <stdio.h>
#include <stdlib.h>
#include "isa.h"
#include "registers.h"
#include "parser.h"
#include "sim.h"
#include "instruction.h"

// Prints usage information
void printUsage()
{
  printf("Usage: sim <tracefile> <# instructions>\n");
}

// Given a Y86-64 tracefile and the number of instructions to simulate in
// the trace file, simulates executing those instructions on an ideal
// pipeline, a stalling pipeline, and a data forwarding pipeline and prints
// out the completion times of those instructions on each pipeline.
int main(int argc, char *argv[])
{
  if(argc < 3){
    printUsage();
    exit(EXIT_FAILURE);
  }

  // Make sure the number of instructions specified is a positive integer
  int numInstrs = atoi(argv[2]);
  if(numInstrs < 1){
    printUsage();
    exit(EXIT_FAILURE);
  }

  // Get the set of instructions from the trace file
  Instruction *instrs = readInstructions(argv[1], numInstrs);


  // Execute and print the results of the ideal pipeline
  InstructionCompletion * noHazardTimes = executeNoHazardPipeline(instrs,
								  numInstrs);
  printCompletionTimes("No hazards", noHazardTimes, numInstrs);
  free(noHazardTimes);

  // Execute and print the results of the stalling pipeline  
  InstructionCompletion * stallTimes = executeStallPipeline(instrs,
							    numInstrs);
  printCompletionTimes("\nStall on hazards", stallTimes, numInstrs);
  free(stallTimes);

  // Execute and print the results of the data forwarding  pipeline  
  InstructionCompletion * forwardTimes = executeForwardPipeline(instrs,
							    numInstrs);
  printCompletionTimes("\nForward on hazards", forwardTimes, numInstrs);
  free(forwardTimes);  

  // Need to free instructions
  freeInstructions(instrs, numInstrs);

}

// Given an array of instructions, execute the specified number of
// instructions on the pipeline and return an array of InstructionCompletion
// instances, one for each of the numInstrs that indicates the instruction's
// completion time.  This pipeline is ideal in that no instructions stall.
InstructionCompletion * executeNoHazardPipeline(Instruction *instrs,
						int numInstrs)
{
  // Create completion time array
  InstructionCompletion *completeTimes =
    (InstructionCompletion*)malloc(numInstrs * sizeof(InstructionCompletion));

  // Initialize an empty pipeline of NUM_STAGES.  Each pipeline stage
  // stores a pointer to the Instruction in that stage or NULL if no
  // instruction is currently in that stage.
  Instruction *pipeline[NUM_STAGES];
  for(int i = 0; i < NUM_STAGES; i++){
    pipeline[i] = NULL;
  }

  // Simulate the pipeline
  int time = 0;
  int completedCount = 0;
  int enteredCount = 0;
  
  // While not all instructions have completed execution
  while(enteredCount < numInstrs || !isPipelineEmpty(pipeline)){
    
    // Move instructions already in the pipeline one stage forward starting at
    // end of pipeline and moving forward to first pipeline stage
    for(int i = NUM_STAGES-1; i >= 0; i--){      
      if(pipeline[i] != NULL){
	// If the pipeline stage is not empty
	
	if(i == PIPE_WRITE_REG_STAGE){
	  // If the pipeline stage is the last stage where we write to
	  // registers, create an InstructionCompletion entry for the
	  // instruction
	  completeTimes[completedCount].instr = pipeline[i];
	  completeTimes[completedCount].completionTime = time;
	  completedCount++;
	  pipeline[i] = NULL;
	}
	else{
	  // If the pipeline stage is not the last stage, just advance
	  // the instruction one stage.  Since we don't recognize hazards
	  // and consequently don't stall instructions, there is no reason
	  // to check if the next pipeline stage is unoccupied before
	  // advancing.
	  pipeline[i+1] = pipeline[i];
	  pipeline[i] = NULL;
	}
      }
    }
    
    if(enteredCount < numInstrs){
      // This handles the insertion of instructions into the first
      // stage of the pipeline.  If there are instructions that
      // haven't entered the pipeline yet, insert the next instruction
      // into the pipeline's first stage.
      pipeline[PIPE_ENTER_STAGE] =  &instrs[enteredCount];
      enteredCount++;
    }

    // Advance time
    time++;
  }

  // Return the array of InstructionCompletion instances
  return completeTimes;
}

// Given an array of instructions, execute the specified number of
// instructions on the pipeline and return an array of InstructionCompletion
// instances, one for each of the numInstrs that indicates the instruction's
// completion time.  This pipeline stalls instructions waiting for their
// source operand registers to be written by an instruction already in the
// pipeline until that instruction has been to the register file.  It also
// stalls on control instructions until the target destination is know.
InstructionCompletion * executeStallPipeline(Instruction *instrs,
					     int numInstrs)
{

  // Create completion time array
  InstructionCompletion *completeTimes =
    (InstructionCompletion*)malloc(numInstrs * sizeof(InstructionCompletion));

  // Initialize an empty pipeline of NUM_STAGES.  Each pipeline stage
  // stores a pointer to the Instruction in that stage or NULL if no
  // instruction is currently in that stage.      
  Instruction *pipeline[NUM_STAGES];
  for(int i = 0; i < NUM_STAGES; i++){
    pipeline[i] = NULL;
  }

  // Simulate the pipeline
  int time = 0;
  int completedCount = 0;
  int enteredCount = 0;

  // While not all instructions have completed execution
  while(enteredCount < numInstrs || !isPipelineEmpty(pipeline)){

    
    // Move instructions already in pipeline one stage forward starting from
    // end of pipeline and moving backwards to first pipeline stage
    for(int i = NUM_STAGES-1; i >= 0; i--){
      if(pipeline[i] != NULL){
        // If the pipeline stage is not empty

	// If the pipeline stage is the last stage where we write to
	// registers, create an InstructionCompletion entry for the        
	// instruction     
        if(i == PIPE_WRITE_REG_STAGE){
          completeTimes[completedCount].instr = pipeline[i];
          completeTimes[completedCount].completionTime = time;
          completedCount++;
          pipeline[i] = NULL;
        }
	else {
	  if (pipeline[i+1] == NULL) {    

	    // if stalling you don't want to do anything else this clock.
	    // break out of current for loop (represents single clock)
	    if (i == PIPE_READ_REG_STAGE && stallOrNot(pipeline, i)){
	      break;
	    }
	 
	    // If the pipeline stage is not the last stage, just advance
	    // the instruction one stage.  Since we don't recognize hazards
	    // and consequently don't stall instructions, there is no reason
	    // to check if the next pipeline stage is unoccupied before         
	    // advancing.
	  
	    pipeline[i+1] = pipeline[i];
	    pipeline[i] = NULL;
	  }
	}  
      }
    }

    if(enteredCount < numInstrs){
      
      // makes sure we can add new instruction to pipeline (no hazard/empty
      // spot at enter stage)
      if (!controlHazard(pipeline) && pipeline[PIPE_ENTER_STAGE] == NULL){
	pipeline[PIPE_ENTER_STAGE] =  &instrs[enteredCount];
	enteredCount++;
      }
    }
  
    time++;
  }

  // Return the array of InstructionCompletion instances             
  return completeTimes;
}

// given the current state of the Pipeline and the stage it is on
// this will check if there are any uses of registers in the
// execute and memory stage that need to be written back before accessing in
// the decode stage
int stallOrNot(Instruction *pipeline[NUM_STAGES], int stage)  
{

  Instruction *current = pipeline[stage];
  
  // set up blank array and fill with src regIDs of current stage
  RegID currentSRegisters[MAX_SRC_REGS];
  int numSrc =  getSrcRegisters(current, currentSRegisters);
  
  // goes through all stages ahead of current stage in pipeline until write
  for (int oldInst = (stage + 1); oldInst <= PIPE_WRITE_REG_STAGE; oldInst++){
      
    Instruction *future = pipeline[oldInst];
      
    // if (!null) then do all below
    if (future != NULL){
      // fill blank array with dest regIDs of stage in question
      RegID futureDRegisters[MAX_DEST_REGS];
      int numDest = getDestRegisters(future, futureDRegisters);	

      // nested loops to see if any elements of dest regIDs array match
      // with any in src regIDs array
      for (int i = 0; i < numDest; i++){
     
	for (int j = 0; j < numSrc; j++){

	  // return 1 if hazard exists
	  if (futureDRegisters[i] == currentSRegisters[j]){
	    return 1;
	  }
	}
      }
    }
  }

  // no hazard detected
  return 0;
}

// returns whether controlHazard exists in pipeline
int controlHazard(Instruction *pipeline[NUM_STAGES]){

  for (int i = PIPE_ENTER_STAGE; i <= PIPE_KNOW_NEXTPC; i++){
    
    if (pipeline[i] == NULL){

      continue;

    } else if (isControlInstruction(pipeline[i]->name)){
      
      return 1;

    }
  }
  return 0;
}

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
					       int numInstrs)
{
  // Create completion time array
  InstructionCompletion *completeTimes =
    (InstructionCompletion*)malloc(numInstrs * sizeof(InstructionCompletion));

  // Initialize an empty pipeline of NUM_STAGES.  Each pipeline stage
  // stores a pointer to the Instruction in that stage or NULL if no
  // instruction is currently in that stage.      
  Instruction *pipeline[NUM_STAGES];
  for(int i = 0; i < NUM_STAGES; i++){
    pipeline[i] = NULL;
  }

  // Simulate the pipeline
  int time = 0;
  int completedCount = 0;
  int enteredCount = 0;
  
  // While not all instructions have completed execution
  while(enteredCount < numInstrs || !isPipelineEmpty(pipeline)){

    // Move instructions in pipeline one stage forward starting from
    // end of pipeline and moving backwards to first stage
    for(int i = NUM_STAGES-1; i >= 0; i--){
      // If the pipeline stage is not empty
      if(pipeline[i] != NULL){

	// If the pipeline stage is the last stage where we write to
	// registers, create an InstructionCompletion entry for the        
	// instruction     
        if(i == PIPE_WRITE_REG_STAGE){
 
          completeTimes[completedCount].instr = pipeline[i];
          completeTimes[completedCount].completionTime = time;
          completedCount++;
          pipeline[i] = NULL;

        } else {

	  if (pipeline[i+1] == NULL) {

	    // do nothing else in this clock if at execute and need to stall
	    if (i == PIPE_READ_FORWARD_REG_STAGE && stallOrNotII(pipeline, i)){
	      break;
	    }
	    
	    pipeline[i+1] =  pipeline[i];
	    pipeline[i] = NULL;    
	    
	  }
	}
      }
    }

    if(enteredCount < numInstrs){
      
      // if no controlHazard and first stage is empty, add new instruction
      // to pipeline
      if (!controlHazard(pipeline) && pipeline[PIPE_ENTER_STAGE] == NULL){
	pipeline[PIPE_ENTER_STAGE] =  &instrs[enteredCount];
	enteredCount++;
	
      }
    }
 
    time++;
  }

  return completeTimes;

}


// given the current state of the Pipeline and the stage it is on
// this will check for need of stalling in data forwarding pipeline
int stallOrNotII(Instruction *pipeline[NUM_STAGES], int stage)
{
  Instruction *current = pipeline[stage];

  // fills blank array with src RegIDs of current stage
  RegID currentSRegisters[MAX_SRC_REGS];
  int numSrc = getSrcRegisters(current, currentSRegisters);

  //goes through all the stages ahead of decode in the pipeline until write
  for (int oldInst = (stage + 1); oldInst <= PIPE_WRITE_REG_STAGE; oldInst++){

    Instruction *future = pipeline[oldInst];

    if (future != NULL){

      // fills blank array with dest RegIDs of stage in question
      RegID futureDRegisters[MAX_DEST_REGS];                                    
      int numDest = getDestRegisters(future, futureDRegisters);
      
      // also check for the operand type;
      OperandType operand = getSourceOperandType(future->name, 0);
	 
      // nested for-loops check that any src RegID != any dest RegID
      for (int i = 0; i < numDest; i++){
	
	for (int j = 0; j < numSrc; j++){

	  // if they have the same registers IDs then there will be a
	  // hazard, so return 1                
	  if (futureDRegisters[i] == currentSRegisters[j]){
              
	    // check if the operand type is memory. If so, has it reached the memory stage.                                            
	    //if neither, then check has it hit the execute stage.

	    if (operand == OP_MEMORY) {
	      // return 1 if stage with dest regID == src regID is before
	      // end of memory stage
	      if (oldInst <= (PIPE_MEMORY_PRODUCE_STAGE + 1)){
		return 1;
	      }
	    }
	    // if operand != OP_MEMORY
	    // return 1 if stage where dest regID == src regID is before
	    // end of execute stage.
	    else {
	      if (oldInst <= (PIPE_ALU_PRODUCE_STAGE + 1)){
		return 1;
	      }
	    }
	  }
	}
      }
    }
  }
  return 0;
}


// Returns 1 if all pipeline stages are empty. Returns 0 otherwise.
int isPipelineEmpty(Instruction *pipeline[])
{
  for(int i = 0; i < NUM_STAGES; i++){
    if(pipeline[i] != NULL){
      return 0;
    }
  }
  return 1;
}

// For a specified pipeline, print out the specified number of instructions
// and their completion times to stdout
void printCompletionTimes(char *pipelineName,
			  InstructionCompletion *instrTimes, int numInstrs)
{
  if(instrTimes == NULL)
    return;
  
  printf("\n%s:\n", pipelineName);

  printf("Instr# \t Addr \t Instruction \t\t\t\tCompletion Time\n");  
  printf("------ \t ---- \t ----------- \t\t\t\t---------------\n");
  for(int i = 0; i < numInstrs; i++){
    char buffer[40];
    char *asmPtr = getInstructionAssembly(instrTimes[i].instr);
    padString(buffer, asmPtr, 40);

    printf("%d:   \t %s \t%d\n", (i+1),   
	   buffer,
	   instrTimes[i].completionTime);
    free(asmPtr);
  }
      
}

