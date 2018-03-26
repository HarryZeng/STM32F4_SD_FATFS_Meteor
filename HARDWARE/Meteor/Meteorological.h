#ifndef __Meteorological_H
#define __Meteorological_H	

#include "sys.h"
#include <stdbool.h>
#include <string.h>

bool CheckCommunication(void);
bool StartMea(void);
bool StopMea(void);
bool ReadMeteorVal (void);

#endif	   
