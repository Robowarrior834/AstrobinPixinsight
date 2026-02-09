"""
Pipeline Processor Module - AstroBin Upload Utility v2.0.0

This module implements the Pipeline Design Pattern, which is the architectural 
core of the v2.0 application. By decoupling complex metadata transformations 
into discrete 'Steps', we ensure that the logic is modular, testable, and 
easy to extend.

Each Step in the pipeline operates on a shared 'SessionState' object, 
modifying it in place or replacing its internal DataFrames as it flows 
through the execution sequence.
"""

import logging
from typing import List, Protocol
from models import SessionState

class PipelineStep(Protocol):
    """
    Interface for a single transformation step in the pipeline.
    
    Any class that implements an 'execute' method accepting and returning 
    a SessionState object is a valid PipelineStep. This protocol-based 
    approach allows for loose coupling between the processor and its logic.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Runs the specific transformation logic for this step.
        
        Args:
            state (SessionState): The current state of the session.
            
        Returns:
            SessionState: The updated session state.
        """
        ...

class PipelineProcessor:
    """
    Orchestrator responsible for chaining and executing pipeline steps.
    
    The processor maintains an ordered list of steps and ensures that 
    the SessionState flows through them sequentially, respecting 
    data dependencies (e.g., normalization must happen before aggregation).
    """
    def __init__(self, logger: logging.Logger):
        """
        Initializes the processor with a system logger.

        Args:
            logger (logging.Logger): The active application logger.
        """
        self.logger = logger
        self.steps: List[PipelineStep] = []

    def add_step(self, step: PipelineStep):
        """
        Registers a new step to be executed in the sequence.
        
        Steps are executed in the order they are added.

        Args:
            step (PipelineStep): An instance of a class implementing the PipelineStep protocol.
        """
        self.logger.debug(f"Registered pipeline step: {step.__class__.__name__}")
        self.steps.append(step)

    def run(self, state: SessionState) -> SessionState:
        """
        Executes all registered steps sequentially on the provided state.
        
        This is the primary execution loop of the application's processing engine.

        Args:
            state (SessionState): The initial data and configuration state.
            
        Returns:
            SessionState: The fully transformed and processed data state.
        """
        self.logger.info(f"Pipeline execution started: {len(self.steps)} steps registered.")
        
        # Sequentially flow the state through each registered transformation
        for step in self.steps:
            self.logger.debug(f"Executing step: {step.__class__.__name__}")
            state = step.execute(state)
            
        self.logger.info("Pipeline execution completed successfully.")
        return state
