//
//	Include for SenCalc
//

/// defines ////////////////////////////////////////////////

enum{
   EXP_OK = 0,
   EXP_ERR_SYNTAX,
   EXP_ERR_INTERNAL1,
   EXP_ERR_INVALIDOPERAND,
   EXP_ERR_INVALIDOPERATOR,
   EXP_ERR_ZERODIVIDE,
   EXP_ERR_NOPARENOPEN,
   EXP_ERR_NOPARENCLOSE,
   EXP_ERR_BIGNUMBER,
   EXP_ERR_INVALIDSTRINGNUMBER,
   };

/// prototypes /////////////////////////////////////////////

int
 Calculator( unsigned char *p, long *result, int *errPos, unsigned char **errMsg );

int
 Calculator64( unsigned char *p, __int64 *result, int *errPos, unsigned char **errMsg );

/// End of module //////////////////////////////////////////
