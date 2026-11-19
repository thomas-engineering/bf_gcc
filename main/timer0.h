
// Typedef for Timer callback function 
typedef void (*TIMER_CALLBACK_FUNC) (void);

#define TIMER0_NUM_CALLBACKS        4
#define TIMER0_NUM_COUNTDOWNTIMERS  4

void Timer0_Init(void);

BOOL Timer0_RegisterCallbackFunction(TIMER_CALLBACK_FUNC pFunc);
BOOL Timer0_RemoveCallbackFunction(TIMER_CALLBACK_FUNC pFunc);


//mt 
// char Timer0_AllocateCountdownTimer();
uint8_t Timer0_AllocateCountdownTimer(void);
uint8_t Timer0_GetCountdownTimer(uint8_t timer);
void Timer0_SetCountdownTimer(uint8_t timer, uint8_t value);
void Timer0_ReleaseCountdownTimer(uint8_t timer);
