"""
Pipeline Processor - AstroBin Upload Utility v2.0.0

Implements the Pipeline Pattern to orchestrate data transformations.
Each logical operation (Deduplication, Matching, etc.) is isolated as a Step.
"""

import logging
from typing import List, Protocol
from models import SessionState

class PipelineStep(Protocol):
    """
    Interface for a single transformation step in the pipeline.
    Ensures that every step accepts a state and returns an updated state.
    """
    def execute(self, state: SessionState) -> SessionState:
        """Runs the transformation logic."""
        ...

class PipelineProcessor:
    """
    Manager class responsible for chaining together and running pipeline steps.
    """
    def __init__(self, logger: logging.Logger):
        self.logger = logger
        self.steps: List[PipelineStep] = []

    def add_step(self, step: PipelineStep):
        """Registers a new step to be executed in the sequence."""
        self.logger.debug(f"Registered pipeline step: {step.__class__.__name__}")
        self.steps.append(step)

    def run(self, state: SessionState) -> SessionState:
        """
        Executes all registered steps sequentially.
        
        Args:
            state (SessionState): The initial data and configuration state.
            
        Returns:
            SessionState: The fully transformed data state.
        """
        self.logger.info(f"Pipeline started: {len(self.steps)} steps.")
        # Sequential processing ensures data dependencies are respected.
        for step in self.steps:
            state = step.execute(state)
        self.logger.info("Pipeline completed.")
        return state