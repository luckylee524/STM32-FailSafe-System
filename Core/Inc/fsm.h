#ifndef INC_FSM_H_
#define INC_FSM_H_

#include "main.h"

typedef enum { ST_INIT = 0, ST_NORMAL, ST_ERROR, ST_SAFE } FsmState_t;

void FSM_Init(void);
void FSM_Run(void);   /* main while(1)에서 매 바퀴 호출 */

#endif /* INC_FSM_H_ */
