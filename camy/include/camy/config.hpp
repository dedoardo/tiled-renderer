#pragma once

/*
	Note:
		Bunch of configuration defines related to how the state validation / error handling
		that is used in the code
*/

/*
	Validation of pipeline states / resources ( Kinda general atm. include various things ) 
*/
#define camy_validate_states 1 << 0

/*
	Asserts will trigger breakpoints
*/
#define camy_enable_asserts 1 << 2

/*
	Basic logging facility, 3 levels
	1 - Info	Successful completion of major operations
	2 - Warning Operation didnt go as expected
	3 - Error   Operation failed
	For critical stuff, usually an assert will exist
*/
#define camy_enable_logging_l1 1 << 3 // Only error
#define camy_enable_logging_l2 (1 << 4 | 1 << 3)// Error & warning
#define camy_enable_logging_l3 (1 << 5 | 1 << 4 | 1 << 3) // All

/*
	Configuration modes are called test or final, but everything can be controlled by the 
	above defines. If you want to modify default or add other modes simply add a case and 
	define the required config parameters
*/
#if !defined(camy_mode_test) || !defined(camy_mode_final)
#define camy_mode_test
#endif

#if defined(camy_mode_test)
#define camy_flags camy_validate_states | camy_enable_asserts | camy_enable_logging_l3
#else
#define camy_flags camy_enable_logging_l1
#endif