# PipelineSimulator
Sarah and Jason's Lab to make a Pipeline Simulation

In order to process commands efficiently, the computer uses a pipeline and its clock to read and write commands. The main phases in the pipe line are: FETCH, DECODE, EXECUTE, MEMORY, and WRITEBACK. In a simple pipeline, you would do these one at a time. Since we are trying to minimize the amount of time it takes, we perform these actions at the same time on different parts of the inputs, like how clothes can be in the wash while other clothes are in the dryer. 

Two extra features in this more complicated pipeline are Data Forwarding and Data Stalling. Sometimes MEMORY phase will use information from the EXECUTE phase, and instead of transfering it via the pipline, we can directly forward the data, which saves time. An issue that can occur is if two instructions depend on each other and are less than 5 apart. In this case we use Data Stalling so that we ensure the correct data is written back (WRITEBACK) before the dependent command fetches is (FETCH). 
