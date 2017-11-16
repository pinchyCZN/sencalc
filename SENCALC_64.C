//
//    SEN's Calculator
//
//    18 Dec 1997 - Based release
//    23 Dec 1997 - Added show output result in Hex
//                - Added shift operation
//    24 Dec 1997 - Added show output result in unsigned, signed, hex
//                - strtoul()
//                - in OpShiftxx() a as unsigned
//    29 Mar 1999 - FIX: divide by zero for modulus
//     6 Aug 2001 - int *errPos, BYTE **errMsg in Calculator()
//

//#define  TEST

#include <setjmp.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <string.h>
#include <errno.h>

#ifdef   TEST
#include <stdio.h>
#endif /*TEST*/

typedef  unsigned char     BYTE;
typedef  unsigned short    WORD;
typedef  unsigned long     DWORD;

#include "sencalc.h"

/// typedefs ///////////////////////////////////////////////

typedef struct{
   __int64  value;
   WORD  type;
   }TOKEN;

typedef struct{
   BYTE  op[ 3 ];
   __int64  (*Operation)( __int64, __int64 );
   WORD  token;
   }OPERATOR64;

/// defines ////////////////////////////////////////////////

#define  NOT_USED                   0L
#define  UNARY_YES                  1
#define  UNARY_NO                   0

#define  EXP_END                    0
#define  EXP_PLUS                   1
#define  EXP_MINUS                  2
#define  EXP_VALID                  8
#define  EXP_NUMBER                 0x0008
#define  EXP_LOGICOR                0x0010
#define  EXP_LOGICAND               0x0020
#define  EXP_BITOR                  0x0040
#define  EXP_BITXOR                 0x0080
#define  EXP_BITAND                 0x0100
#define  EXP_EQUAL                  0x0200
#define  EXP_EQUAL_EQUAL            EXP_EQUAL
#define  EXP_EQUAL_NOTEQUAL         EXP_EQUAL+1
#define  EXP_RELATION               0x0400
#define  EXP_RELATION_LESS          EXP_RELATION
#define  EXP_RELATION_GREAT         EXP_RELATION+1
#define  EXP_RELATION_LESSEQ        EXP_RELATION+2
#define  EXP_RELATION_GREATEQ       EXP_RELATION+3
#define  EXP_ADDITIVE               0x0800
#define  EXP_ADDITIVE_PLUS          EXP_ADDITIVE+EXP_PLUS
#define  EXP_ADDITIVE_MINUS         EXP_ADDITIVE+EXP_MINUS
#define  EXP_MULTI                  0x1000
#define  EXP_MULTI_MUL              EXP_MULTI
#define  EXP_MULTI_DIV              EXP_MULTI+1
#define  EXP_MULTI_MODULUS          EXP_MULTI+2
#define  EXP_PAREN                  0x2000
#define  EXP_PAREN_R                EXP_PAREN
#define  EXP_PAREN_L                EXP_PAREN+1
#define  EXP_UNARY                  0x4000
#define  EXP_UNARY_PLUS             EXP_UNARY+EXP_PLUS
#define  EXP_UNARY_MINUS            EXP_UNARY+EXP_MINUS
#define  EXP_UNARY_NEG              EXP_UNARY+3
#define  EXP_UNARY_NOT              EXP_UNARY+4
#define  EXP_SHIFT                  0x8000
#define  EXP_SHIFT_R                EXP_SHIFT
#define  EXP_SHIFT_L                EXP_SHIFT+1

/// prototypes /////////////////////////////////////////////

BYTE
         *Exp( BYTE *p, BYTE* (*NextLevel)( BYTE* ), WORD type ),
  *Expression( BYTE *p ),
   *ExpAdding( BYTE *p ),
   *ExpBitAnd( BYTE *p ),
    *ExpBitOr( BYTE *p ),
   *ExpBitXor( BYTE *p ),
    *ExpEqual( BYTE *p ),
 *ExpLogicAnd( BYTE *p ),
  *ExpLogicOr( BYTE *p ),
    *ExpMulti( BYTE *p ),
  *ExpOperand( BYTE *p ),
    *ExpRelat( BYTE *p ),
    *ExpShift( BYTE *p ),
    *ExpToken( BYTE *p, short unary_flag ),
    *ExpValue( BYTE *p ),
   *SkipSpace( BYTE *p );
BYTE
 *Str2Dword( BYTE *s, __int64 *a ),
 *Text2Dword( BYTE *s, __int64 *val, int base ),
  Text2DwordLastChar( BYTE *s );
int
 OperatorIndex( WORD type ),
    TokenIndex( BYTE *p );

/// global variables ///////////////////////////////////////

static BYTE
  *error[]={ "",
             "Syntax error",
             "Internal error",
             "Invalid operand",
             "Invalid operator",
             "Divide by zero",
             "Missing open paren",
             "Missing close paren",
             "Big number",
             "Long string number",
            },
 *ExpErrPoint;
TOKEN
 token;
jmp_buf
 jmp;

/// operations /////////////////////////////////////////////

__int64      OpError64( __int64 a, __int64 b ){ longjmp( jmp, EXP_ERR_INTERNAL1 ); return( 0 ); };
__int64        OpAdd64( __int64 a, __int64 b ){ return( a + b ); };
__int64        OpSub64( __int64 a, __int64 b ){ return( a - b ); };
__int64      OpMulti64( __int64 a, __int64 b ){ return( a * b ); };
__int64        OpDiv64( __int64 a, __int64 b ){ if( b ) return( a / b ); else longjmp( jmp, EXP_ERR_ZERODIVIDE ); };
__int64  OpShiftLeft64( __int64 a, __int64 b ){ return( (unsigned __int64)a << b ); };
__int64 OpShiftRight64( __int64 a, __int64 b ){ return( (unsigned __int64)a >> b ); };
__int64    OpModulus64( __int64 a, __int64 b ){ if( b ) return( a % b ); else longjmp( jmp, EXP_ERR_ZERODIVIDE ); };
__int64       OpLess64( __int64 a, __int64 b ){ return( a < b ); };
__int64      OpGreat64( __int64 a, __int64 b ){ return( a > b ); };
__int64     OpBitAnd64( __int64 a, __int64 b ){ return( a & b ); };
__int64     OpBitXor64( __int64 a, __int64 b ){ return( a ^ b ); };
__int64      OpBitOr64( __int64 a, __int64 b ){ return( a | b ); };
__int64     OpLessEq64( __int64 a, __int64 b ){ return( a <= b ); };
__int64    OpGreatEq64( __int64 a, __int64 b ){ return( a >= b ); };
__int64      OpEqual64( __int64 a, __int64 b ){ return( a == b ); };
__int64   OpNotEqual64( __int64 a, __int64 b ){ return( a != b ); };
__int64   OpLocicAnd64( __int64 a, __int64 b ){ return( a && b ); };
__int64    OpLogicOr64( __int64 a, __int64 b ){ return( a || b ); };
__int64     OpBitNot64( __int64 a, __int64 b ){ return( ~a ); };
__int64   OpLogicNot64( __int64 a, __int64 b ){ return( !a ); };
__int64 OpUnaryMinus64( __int64 a, __int64 b ){ return( -a ); };
__int64  OpUnaryPlus64( __int64 a, __int64 b ){ return( a ); };

/// operators //////////////////////////////////////////////

static OPERATOR64
 operator[]={ { "",   OpError64,      EXP_END },
                                          // single character FIRST !
              { ")",  OpError64,      EXP_PAREN_R },
              { "(",  OpError64,      EXP_PAREN_L },
              { "!",  OpLogicNot64,   EXP_UNARY_NOT },
              { "~",  OpBitNot64,     EXP_UNARY_NEG },
              { "+",  OpUnaryPlus64,  EXP_UNARY_PLUS },      // unary operation
              { "+",  OpAdd64,        EXP_ADDITIVE_PLUS },    // binary operation
              { "+",  OpError64,      EXP_PLUS },              // token
              { "-",  OpUnaryMinus64, EXP_UNARY_MINUS },     // unary operation
              { "-",  OpSub64,        EXP_ADDITIVE_MINUS },   // binary operation
              { "-",  OpError64,      EXP_MINUS },             // token
              { "*",  OpMulti64,      EXP_MULTI_MUL },
              { "/",  OpDiv64,        EXP_MULTI_DIV },
              { "%",  OpModulus64,    EXP_MULTI_MODULUS },
              { "<",  OpLess64,       EXP_RELATION_LESS },
              { ">",  OpGreat64,      EXP_RELATION_GREAT },
              { "&",  OpBitAnd64,     EXP_BITAND },
              { "^",  OpBitXor64,     EXP_BITXOR },
              { "|",  OpBitOr64,      EXP_BITOR },
                                          // double characters
              { "<<", OpShiftLeft64,  EXP_SHIFT_L },
              { ">>", OpShiftRight64, EXP_SHIFT_R },
              { "<=", OpLessEq64,     EXP_RELATION_LESSEQ },
              { ">=", OpGreatEq64,    EXP_RELATION_GREATEQ },
              { "==", OpEqual64,      EXP_EQUAL_EQUAL },
              { "!=", OpNotEqual64,   EXP_EQUAL_NOTEQUAL },
              { "&&", OpLocicAnd64,   EXP_LOGICAND },
              { "||", OpLogicOr64,    EXP_LOGICOR } };

////////////////////////////////////////////////////////////

#ifdef   TEST
void
 main( int argc, BYTE **argv )
{
 BYTE
  *errMsg;
 int
   errPos;
 long
   result;

 if( argc > 1 ){
   do{
     printf( "\"%s\"", *++argv );
     if( Calculator( *argv, &result, &errPos, &errMsg ) )
       printf( "\n %*s^ %s\n", errPos, "",  errMsg );
      else
       printf( " = %ld (+%lu 0x%8.8lX)\n", result, result, result );
   }while( --argc > 1 );
 }else
   puts( "Usage: SENCALC expression1 [expression2] [expression3] ..." );
}
#endif /*TEST*/

////////////////////////////////////////////////////////////

int
 Calculator64( BYTE *p, __int64 *result, int *errPos, BYTE **errMsg )
{
 int
   rc=setjmp( jmp );

 if( rc == EXP_OK ){
   if( *Expression( p ) )
     longjmp( jmp, EXP_ERR_NOPARENOPEN );
   *result=token.value;
   if( errPos )
     *errPos=0;
 }else
   if( errPos )
     *errPos=ExpErrPoint-p;
 if( errMsg )
   *errMsg=error[ rc ];
 return( rc );
}

////////////////////////////////////////////////////////////

static BYTE
 *Expression( BYTE *p )
{
 return( ExpLogicOr( p ) );            // start at lower priority
}

////////////////////////////////////////////////////////////

static BYTE  *ExpLogicOr( BYTE *p ){ return( Exp( p, ExpLogicAnd, EXP_LOGICOR ) ); }
static BYTE *ExpLogicAnd( BYTE *p ){ return( Exp( p, ExpBitOr,    EXP_LOGICAND ) ); }
static BYTE    *ExpBitOr( BYTE *p ){ return( Exp( p, ExpBitXor,   EXP_BITOR ) ); }
static BYTE   *ExpBitXor( BYTE *p ){ return( Exp( p, ExpBitAnd,   EXP_BITXOR ) ); }
static BYTE   *ExpBitAnd( BYTE *p ){ return( Exp( p, ExpEqual,    EXP_BITAND ) ); }
static BYTE    *ExpEqual( BYTE *p ){ return( Exp( p, ExpRelat,    EXP_EQUAL ) ); }
static BYTE    *ExpRelat( BYTE *p ){ return( Exp( p, ExpShift,    EXP_RELATION ) ); }
static BYTE    *ExpShift( BYTE *p ){ return( Exp( p, ExpAdding,   EXP_SHIFT ) ); }
static BYTE   *ExpAdding( BYTE *p ){ return( Exp( p, ExpMulti,    EXP_ADDITIVE ) ); }
static BYTE    *ExpMulti( BYTE *p ){ return( Exp( p, ExpOperand,  EXP_MULTI ) ); }

////////////////////////////////////////////////////////////

static BYTE
 *ExpOperand( BYTE *p )
{
 p=ExpValue( p );
 if( token.type != EXP_NUMBER )
   longjmp( jmp, EXP_ERR_SYNTAX );
 p=ExpToken( p, UNARY_NO );
 if( token.type == EXP_NUMBER )
   longjmp( jmp, EXP_ERR_SYNTAX );
 return( p );
}

////////////////////////////////////////////////////////////

static BYTE
 *ExpValue( BYTE *p )
{
 TOKEN
   tokenOld;

 p=ExpToken( p, UNARY_YES );
 memcpy( &tokenOld, &token, sizeof( TOKEN ) );
 if( tokenOld.type & EXP_UNARY ){
   p=ExpValue( p );
   if( token.type & EXP_NUMBER )
     token.value=operator[ OperatorIndex( tokenOld.type ) ].Operation( token.value, NOT_USED );
    else
     longjmp( jmp, EXP_ERR_INVALIDOPERAND );
 }else{
   if( token.type == EXP_PAREN_L ){
     p=Expression( p );
     if( ( token.type == EXP_PAREN_R ) == 0 )
       longjmp( jmp, EXP_ERR_NOPARENCLOSE );
      else
       token.type=EXP_NUMBER;
   }
 }
 return( p );
}

////////////////////////////////////////////////////////////

static BYTE
 *ExpToken( BYTE *p, short unary_flag )
{
 short
   index;

 ExpErrPoint=p=SkipSpace( p );
 if( *p ){
   if( isxdigit( *p ) || *p == '"' ){
     p=Text2Dword( p, &token.value, 10 );
     if( p == NULL )
       longjmp( jmp, EXP_ERR_INVALIDSTRINGNUMBER );
     token.type=EXP_NUMBER;
   }else{
     if( index=TokenIndex( p ) ){
       token.type=operator[ index ].token;
       p+=strlen( operator[ index ].op );
       if( token.type < EXP_VALID )
         token.type+=unary_flag? EXP_UNARY : EXP_ADDITIVE;
     }else
       longjmp( jmp, EXP_ERR_INVALIDOPERATOR );
   }
 }else
   token.type=EXP_END;
 return( p );
}

////////////////////////////////////////////////////////////

static BYTE
 *Exp( BYTE *p, BYTE* (*NextLevel)( BYTE* ), WORD type )
{
 TOKEN
   tokenOld;

 p=NextLevel( p );
 while( token.type & type ){
   memcpy( &tokenOld, &token, sizeof( TOKEN ) );
   p=NextLevel( p );
   token.value=operator[ OperatorIndex( tokenOld.type ) ].Operation( tokenOld.value, token.value );
 }
 return( p );
}

////////////////////////////////////////////////////////////

static int
 TokenIndex( BYTE *p )
{
 int
   i=sizeof( operator )/sizeof( OPERATOR64 );

 while( --i > 0 && memcmp( operator[ i ].op, p, strlen( operator[ i ].op ) ) )
   ;
 return( i );
}

////////////////////////////////////////////////////////////

static int
 OperatorIndex( WORD type )
{
 int
   i=sizeof( operator )/sizeof( OPERATOR64 );

 while( --i > 0 && operator[ i ].token != type )
   ;
 return( i );
}

////////////////////////////////////////////////////////////

static BYTE
 *SkipSpace( BYTE *p )
{
 while( isspace( *p ) )
   p++;
 return( p );
}


unsigned __int64 _strtoui64(BYTE *s,BYTE **end,int base)
{
#define FALSE 0
#define TRUE 1
	unsigned __int64 result=0;
	int neg=FALSE;
	*end=s;
	while(1){
		BYTE c=*s;
		if(c==' '){
			*end=s;
			s++;
			continue;
		}else
			break;
	}
	{
		BYTE c=*s;
		if(c=='-'){
			neg=TRUE;
			s++;
		}else if(c=='+'){
			s++;
		}
	}
	while(1){
		BYTE c=*s;
		*end=s;
		switch(base){
		case 2:
			if(c=='1' || c=='0'){
				if((result>>63)&1){
					errno=ERANGE;
					goto EXIT;
				}
				result<<=1;
				result+=c-'0';
			}else{
				goto EXIT;
			}
			break;
		case 8:
			if(c>='0' && c<='7'){
				if((result>>61)&7){
					errno=ERANGE;
					goto EXIT;
				}
				result*=8;
				result+=c-'0';
			}else{
				goto EXIT;
			}
			break;
		case 10:
			if(c>='0' && c<='9'){
				unsigned __int64 tmp;
				tmp=result*10;
				tmp=tmp/10;
				if(tmp!=result){
					errno=ERANGE;
					goto EXIT;
				}
				result*=10;
				result+=c-'0';
			}else{
				goto EXIT;
			}
			break;
		case 16:
			c=toupper(c);
			if((c>='0' && c<='9') || (c>='A' && c<='F')){
				int val;
				if((result>>60)&0xF){
					errno=ERANGE;
					goto EXIT;
				}
				if(c>='0' && c<='9')
					val=c-'0';
				else
					val=c-'A'+10;
				result*=16;
				result+=val;
			}else{
				goto EXIT;
			}
			break;
		default:
			errno=ERANGE;
			goto EXIT;
			break;
		}
		if(0==c)
			break;
		s++;
	}
EXIT:
	if(neg)
		result=-result;
	return result;
}
////////////////////////////////////////////////////////////

static BYTE
 *Text2Dword( BYTE *s, __int64 *val, int base )
{
 BYTE
   suffix;
 int
   prefix=0;

 if( *s ){
   if( *s == '"' )
     s=Str2Dword( s, val );
    else{
     if( base ){
       if( ( *(WORD*)s | 0x2020 ) == 0x7830 ){
         prefix=2;
         base=16;
       }
       switch( suffix=Text2DwordLastChar( s+prefix ) ){
         case 'i': case 'I': base=2;  break;
         case 'o': case 'O': base=8;  break;
         case 't': case 'T': base=10; break;
         case 'h': case 'H': base=16; break;
         default:  suffix=0;
       }
     }
     errno=0;
	 *val=_strtoui64(s+prefix,&s,base);
     //*val=strtoul( s+prefix, &s, base );
     if( errno == ERANGE )
       longjmp( jmp, EXP_ERR_BIGNUMBER );
     if( suffix && *s == suffix )
       s++;
   }
 }else
   *val=0L;
 return( s );
}

////////////////////////////////////////////////////////////

static BYTE
 Text2DwordLastChar( BYTE *s )
{
 if( *s == '-' || *s == '+' )
   s++;
 while( isxdigit( *s ) )
   s++;
 return( *s );
}

////////////////////////////////////////////////////////////

static BYTE
 *Str2Dword( BYTE *s, __int64 *a )
{
 *a=0;
 while( s && *++s && *s != '"' ){
   if( *a & 0xFF00000000000000 )
     s=NULL;
    else
     *a=( *a << 8 ) | ( ( *s == '\\' )? *++s : *s );
 }
 if( s && *s++ != '"' )
   s=NULL;
 return( s );
}

/// End of module //////////////////////////////////////////
