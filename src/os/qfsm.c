#include "qfsm.h"

#if ( Q_FSM == 1 )

static void qStateMachine_ExecSubStateIfAvailable( const qSM_SubState_t substate, qSM_Handler_t handle );
static void qStateMachine_ExecStateIfAvailable( qSM_t * const obj, const qSM_State_t state );
/*============================================================================*/
/*qBool_t qStateMachine_Setup(qSM_t * const obj, qSM_State_t InitState, qSM_ExState_t SuccessState, qSM_ExState_t FailureState, qSM_ExState_t UnexpectedState, qSM_SubState_t BeforeAnyState);

Initializes a finite state machine (FSM).

Parameters:

    - obj : a pointer to the FSM object.
    - InitState : The first state to be performed. This argument is a pointer 
                  to a callback function, returning qSM_Status_t and with a 
                  qSM_Handler_t as input argument.
    - SuccessState : Sub-State performed after a state finish with return status 
                     qSM_EXIT_SUCCESS. This argument is a pointer to a callback
                     function with a qSM_Handler_t pointer as input argument.
    - FailureState : Sub-State performed after a state finish with return status 
                     qSM_EXIT_FAILURE. This argument is a pointer to a callback
                     function with a qSM_Handler_t as input argument.
    - UnexpectedState : Sub-State performed after a state finish with return status
                        value between -32766 and 32767. This argument is a 
                        pointer to a callback function with a qSM_Handler_t 
                        as input argument.
    - BeforeAnyState : A state called before the normal state machine execution,                   

Return value:

    Returns qTrue on success, otherwise returns qFalse;
*/
qBool_t qStateMachine_Setup( qSM_t * const obj, qSM_State_t InitState, qSM_SubState_t SuccessState, qSM_SubState_t FailureState, qSM_SubState_t UnexpectedState, qSM_SubState_t BeforeAnyState ){
    qBool_t RetValue = qFalse;
    if( ( NULL != obj ) && ( NULL != InitState ) ){
        obj->qPrivate.xPublic.NextState = InitState;
        obj->qPrivate.xPublic.PreviousState = NULL;
        obj->qPrivate.xPublic.Signal  = QSM_SIGNAL_NONE;
        obj->qPrivate.xPublic.PreviousReturnStatus = qSM_EXIT_SUCCESS;
        obj->qPrivate.xPublic.LastState = NULL;
        obj->qPrivate.xPublic.Signal = (qSM_Signal_t)0u;
        obj->qPrivate.Failure = FailureState;
        obj->qPrivate.Success = SuccessState;
        obj->qPrivate.Unexpected = UnexpectedState;
        obj->qPrivate.BeforeAnyState = BeforeAnyState;
        obj->qPrivate.TransitionTable = NULL;
        obj->qPrivate.Owner = NULL;
        obj->qPrivate.SignalQueue.qPrivate.pHead = NULL;
        RetValue = qTrue;
    }
    return RetValue;
}
/*============================================================================*/
static void qStateMachine_ExecSubStateIfAvailable( const qSM_SubState_t substate, qSM_Handler_t handle ){
    if( NULL != substate ){
        substate( handle );
    }
}
/*============================================================================*/
/*void qStateMachine_Run(qSM_t * const obj, void* Data)

Execute the Finite State Machine (FSM).

Parameters:

    - obj : a pointer to the FSM object.
    - Signal : The signal data used to produce a state changes if a 
              transition table is available.
    - Data : Represents the FSM arguments. All arguments must be passed by 
             reference and cast to (void *). Only one argument is allowed, so,
             for multiple arguments, create a structure that contains all of 
             the arguments and pass a pointer to that structure.

Return value:

    The return status of the current FSM state. 
*/    
qSM_Status_t qStateMachine_Run( qSM_t * const obj, void *Data ){
    qSM_State_t CurrentState; 
    qSM_Status_t RetValue = qSM_EXIT_FAILURE;

    if( NULL != obj ){
        obj->qPrivate.xPublic.Data = Data;   /*pass the data through the fsm*/
        obj->qPrivate.xPublic.Signal = QSM_SIGNAL_NONE;

        CurrentState = obj->qPrivate.xPublic.NextState;
        if( obj->qPrivate.xPublic.LastState != CurrentState ){ /*if StateFistEntry is set, update the PreviousState*/
            obj->qPrivate.xPublic.PreviousState = obj->qPrivate.xPublic.LastState ; 
            obj->qPrivate.xPublic.PreviousReturnStatus = obj->qPrivate.xPublic.LastReturnStatus;
            obj->qPrivate.xPublic.Signal = QSM_SIGNAL_ENTRY;
            qStateMachine_ExecStateIfAvailable( obj, CurrentState );
        }
        else{
            if( QSM_SIGNAL_NONE == obj->qPrivate.xPublic.Signal ){
                if( qTrue == qQueue_IsReady( &obj->qPrivate.SignalQueue ) ){
                    if( qTrue == qQueue_Receive( &obj->qPrivate.SignalQueue, &obj->qPrivate.xPublic.Signal ) ){
                        (void)qStateMachine_SweepTransitionTable( obj ); /*sweep the table if available*/
                    }
                }
            }  
            qStateMachine_ExecStateIfAvailable( obj, CurrentState  );
            
            if( CurrentState != obj->qPrivate.xPublic.NextState ){ /*a transition has been performed?*/
                obj->qPrivate.xPublic.Signal = QSM_SIGNAL_EXIT; 
                qStateMachine_ExecStateIfAvailable( obj, CurrentState );
            }
        }
    }
    return RetValue;
 }
/*============================================================================*/
static void qStateMachine_ExecStateIfAvailable( qSM_t * const obj, const qSM_State_t state ){
    qSM_Status_t ExitStatus = qSM_EXIT_FAILURE;
    qSM_Handler_t handle;

    handle = &obj->qPrivate.xPublic;
    qStateMachine_ExecSubStateIfAvailable( obj->qPrivate.BeforeAnyState , handle ); /*eval the BeforeAnyState if available*/
    if( NULL != state ){ /*eval the state if available*/
        ExitStatus = state( handle );
    }
    obj->qPrivate.xPublic.LastReturnStatus = ExitStatus;
    obj->qPrivate.xPublic.LastState = state; /*update the LastState*/
    /*Check return status to eval extra states*/
    if( qSM_EXIT_FAILURE == ExitStatus ){
        qStateMachine_ExecSubStateIfAvailable( obj->qPrivate.Failure, handle ); /*Run failure state if available*/
    }
    else if ( qSM_EXIT_SUCCESS == ExitStatus ){
        qStateMachine_ExecSubStateIfAvailable( obj->qPrivate.Success, handle ); /*Run success state if available*/
    } 
    else{
        qStateMachine_ExecSubStateIfAvailable( obj->qPrivate.Unexpected, handle ); /*Run unexpected state if available*/
    }

}
/*============================================================================*/
/*void qStateMachine_Attribute( qSM_t * const obj, const qFSM_Attribute_t Flag , qSM_State_t  s, qSM_SubState_t subs )

Change attributes or set actions to the Finite State Machine (FSM).

Parameters:

    - obj : a pointer to the FSM object.
    - Flag: The attribute/action to be taken
         > qSM_RESTART : Restart the FSM (val argument must correspond to the init state)
         > qSM_CLEAR_STATE_FIRST_ENTRY_FLAG: clear the entry flag for the 
                current state if the NextState field doesn't change.
         > qSM_FAILURE_STATE: Set the Failure State
         > qSM_SUCCESS_STATE: Set the Success State
         > qSM_UNEXPECTED_STATE: Set the Unexpected State
         > qSM_BEFORE_ANY_STATE: Set the state executed before any state.
         > qSM_UNISTALL_TRANSTABLE : To uninstall the transition table if available
    - s : The new value for state (only apply in qSM_RESTART). If not used, pass NULL.
    - subs : The new value for SubState (only apply in qSM_FAILURE_STATE, qSM_SUCCESS_STATE, 
             qSM_UNEXPECTED_STATE, qSM_BEFORE_ANY_STATE). If not used, pass NULL.
*/    
void qStateMachine_Attribute( qSM_t * const obj, const qFSM_Attribute_t Flag , qSM_State_t  s, qSM_SubState_t subs ){
    if( NULL != obj){
        switch(Flag){
            case qSM_RESTART:
                obj->qPrivate.xPublic.NextState = s;
                obj->qPrivate.xPublic.PreviousState = NULL;
                obj->qPrivate.xPublic.LastState = NULL;
                obj->qPrivate.xPublic.Signal = QSM_SIGNAL_NONE;
                obj->qPrivate.xPublic.PreviousReturnStatus = qSM_EXIT_SUCCESS;            
                break;
            case qSM_CLEAR_STATE_FIRST_ENTRY_FLAG:
                obj->qPrivate.xPublic.PreviousState = NULL;
                obj->qPrivate.xPublic.LastState = NULL;
                break;
            case qSM_FAILURE_STATE:
                obj->qPrivate.Failure = subs;        /*MISRAC2004-11.1 deviation allowed*/
                break;
            case qSM_SUCCESS_STATE:
                obj->qPrivate.Success = subs;        /*MISRAC2004-11.1 deviation allowed*/
                break;    
            case qSM_UNEXPECTED_STATE:
                obj->qPrivate.Unexpected = subs;     /*MISRAC2004-11.1 deviation allowed*/
                break;   
            case qSM_BEFORE_ANY_STATE:
                obj->qPrivate.BeforeAnyState = subs; /*MISRAC2004-11.1 deviation allowed*/
                break;              
            case qSM_UNINSTALL_TRANSTABLE:
                obj->qPrivate.TransitionTable = NULL;
                break;   
            default:
                break;
        }        
    }
}
/*============================================================================*/
/*qBool_t qStateMachine_TransitionTableInstall( qSM_t * const obj, qSM_TransitionTable_t *table, qSM_Transition_t *entries, size_t NoOfEntries, qSM_Signal_t *AxSignals, size_t MaxSignals)

Install a transition table for the supplied state machine instance.

Parameters:

    - obj : a pointer to the FSM object.
    - table: a pointer to the transition table instance
    - entries : The array of transition (qSM_Transition_t[]).
    - NoOfEntries : The number of transitions inside <entries>

Return value:

    Returns qTrue on success, otherwise returns qFalse;
*/  
qBool_t qStateMachine_TransitionTableInstall( qSM_t * const obj, qSM_TransitionTable_t *table, qSM_Transition_t *entries, size_t NoOfEntries ){
    qBool_t RetValue = qFalse;
    if( (NULL != obj) && ( NULL != table ) && ( NULL != entries) && (NoOfEntries > (size_t)0 ) ){
        table->qPrivate.NumberOfEntries = NoOfEntries;
        table->qPrivate.Transitions = entries;
        obj->qPrivate.TransitionTable = table;
        RetValue = qTrue;
    }
    return RetValue;
}
/*============================================================================*/
/*qBool_t qStateMachine_SignalQueueSetup( qSM_t * const obj, qSM_Signal_t *AxSignals, size_t MaxSignals )

Setup the state-machine signal queue.

Parameters:

    - obj : a pointer to the FSM object.
    - AxSignals : A pointer to the memory area used for queueing signals. qSM_Signal_t[MaxSignals]
    - MaxSignals : The number of items inside AxSignals.

Return value:

    Returns qTrue on success, otherwise returns qFalse;
*/ 
qBool_t qStateMachine_SignalQueueSetup( qSM_t * const obj, qSM_Signal_t *AxSignals, size_t MaxSignals ){
    qBool_t RetValue = qFalse;
    if( ( NULL != AxSignals ) && ( MaxSignals > (size_t)0u ) ){
        RetValue = qQueue_Setup( &obj->qPrivate.SignalQueue, AxSignals, sizeof(qSM_Signal_t), MaxSignals );
    }
    return RetValue;
}
/*============================================================================*/
/* qBool_t qStateMachine_SweepTable( qSM_t * const obj )

Forces a sweep over the installed transition table. The instance will be updated 
if a transition from the table is performed.

Note : This method is performed from the qStateMachine_Run() API before the 
current state callback is invoked.

Parameters:

    - obj : a pointer to the FSM object.

Return value:

    Returns qTrue if the sweep produce a state transition, otherwise returns qFalse;
*/
qBool_t qStateMachine_SweepTransitionTable( qSM_t * const obj ){
    qBool_t RetValue = qFalse;
    qSM_TransitionTable_t *table;
    qSM_Transition_t iTransition;
    qSM_State_t xCurrentState;
    size_t iEntry;
    qSM_Signal_t signal;

    if( NULL != obj ){
        table = obj->qPrivate.TransitionTable; /*MISRAC2012-Rule-11.5 deviation allowed*/
        if( NULL != table ){
            signal = obj->qPrivate.xPublic.Signal;
            if( signal < QSM_SIGNAL_RANGE_MAX){
                xCurrentState = obj->qPrivate.xPublic.NextState;
                for( iEntry = 0; iEntry < table->qPrivate.NumberOfEntries; iEntry++ ){
                    iTransition = table->qPrivate.Transitions[iEntry];
                    if( ( xCurrentState == iTransition.xCurrentState ) && ( signal == iTransition.Signal) ){ /*both conditions match*/
                        if( NULL != iTransition.SignalAction ){
                            iTransition.SignalAction();
                        }
                        obj->qPrivate.xPublic.NextState = iTransition.xNextState;    /*make the transition to the target state*/
                        RetValue = qTrue;
                        break; 
                    } 
                }
            }
        }        
    }

    return RetValue;
}
/*============================================================================*/
/*qBool_t qStateMachine_SendSignal( qSM_t * const obj, qSM_Signal_t signal, qBool_t isUrgent )

Sends a signal to the state machine.
Note : The state machine instance must have a transition table previously installed

Parameters:

    - obj : a pointer to the FSM object.
    - signal : The user-defined signal
    - isUrgent : If qTrue, the signal will be sent to the front of the transition-table queue.

Return value:

    qTrue if the provided signal was successfully delivered to the FSM, otherwise return qFalse.

*/
qBool_t qStateMachine_SendSignal( qSM_t * const obj, qSM_Signal_t signal, qBool_t isUrgent ){
    qBool_t RetValue = qFalse;
    if( ( NULL != obj ) && ( QSM_SIGNAL_NONE != signal ) ){
        if( qTrue == qQueue_IsReady( &obj->qPrivate.SignalQueue ) ){
            RetValue = qQueue_SendGeneric( &obj->qPrivate.SignalQueue, &signal, (qQueue_Mode_t)isUrgent );  
        }
    }
    return RetValue;
}
/*============================================================================*/
/*qSM_Handler_t qStateMachine_Get_Handler( qSM_t * const obj )

Return the state-machine handler.

Parameters:

    - obj : a pointer to the FSM object.

Return value:

    The requested FSM handler. NULL if the handler cant be retrieved.

*/
qSM_Handler_t qStateMachine_Get_Handler( qSM_t * const obj ){
    qSM_Handler_t h =  NULL;
    if( NULL != obj ){
        h = &obj->qPrivate.xPublic;
    }
    return h;
}
/*============================================================================*/
/*void qStateMachine_Set_Parent( qSM_t * const Child, qSM_t * const Parent)

Set the state-machine parent.

Parameters:

    - Child : a pointer to the child FSM object.
    - Parent : a pointer to the parent FSM object.

Return value:

    The requested FSM handler. NULL if the handler cant be retrieved.

*/
void qStateMachine_Set_Parent( qSM_t * const Child, qSM_t * const Parent ){
    if( NULL != Child ){
        Child->qPrivate.xPublic.Parent = Parent;
    }
}
/*============================================================================*/
#endif /* #if ( Q_FSM == 1 ) */
