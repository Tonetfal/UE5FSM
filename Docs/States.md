# States

States are the crucial part of the Finite State Machine, they're like bricks you build your functionality upon. A 
state machine can have any number of states which are used for a variety of tasks. Each one of them is meant to be 
specialized in a certain task - Patrolling, Attacking, Dying, and many others. You can freely switch between them 
back and forth depending on your specific needs.

## Types

There are two different types of states: [normal](NormalState.md) and [global](GlobalState.md). Regardless of the state 
type, each one of them has something in common: [State Data](StateData.md) and [Labels](Labels.md).
