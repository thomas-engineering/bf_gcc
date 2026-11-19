#define TEMPERATURE_SENSOR  0
#define VOLTAGE_SENSOR      1
#define LIGHT_SENSOR        2
//#define CELCIUS             3
//#define FARENHEIT           4
#define CELSIUS             3
#define FAHRENHEIT           4

//#define Vref                2.85

void ADC_init(char );
int ADC_read(void);
void ADC_periphery(void);

// Periodic measurement: the Timer0 ISR only sets a flag, ADC_Service() (main loop)
// does the conversion.
void ADC_StartAuto(void);
void ADC_StopAuto(void);
void ADC_Service(void);

// Temperature sensor function
char TemperatureFunc(char);

// Voltage reader function
char VoltageFunc(char);

// Light sensor function
char LightFunc(char);
