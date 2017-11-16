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
   long  value;
   WORD  type;
   }TOKEN;

typedef struct{
   BYTE  op[ 3 ];
   long  (*Operation)( long, long );
   WORD  token;
   }OPERATOR;

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
 *Str2Dword( BYTE *s, long *a ),
 *Text2Dword( BYTE *s, DWORD *val, int base ),
  Text2DwordLastChar( BYTE *s );
int
 OperatorIndex( WORD type ),
    TokenIndex( BYTE *p );

/// global variables ///////////////////////////////////////

BYTE
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

long      OpError( long a, long b ){ longjmp( jmp, EXP_ERR_INTERNAL1 ); return( 0 ); };
long        OpAdd( long a, long b ){ return( a + b ); };
long        OpSub( long a, long b ){ return( a - b ); };
long      OpMulti( long a, long b ){ return( a * b ); };
long        OpDiv( long a, long b ){ if( b ) return( a / b ); else longjmp( jmp, EXP_ERR_ZERODIVIDE ); };
long  OpShiftLeft( long a, long b ){ return( (DWORD)a << b ); };
long OpShiftRight( long a, long b ){ return( (DWORD)a >> b ); };
long    OpModulus( long a, long b ){ if( b ) return( a % b ); else longjmp( jmp, EXP_ERR_ZERODIVIDE ); };
long       OpLess( long a, long b ){ return( a < b ); };
long      OpGreat( long a, long b ){ return( a > b ); };
long     OpBitAnd( long a, long b ){ return( a & b ); };
long     OpBitXor( long a, long b ){ return( a ^ b ); };
long      OpBitOr( long a, long b ){ return( a | b ); };
long     OpLessEq( long a, long b ){ return( a <= b ); };
long    OpGreatEq( long a, long b ){ return( a >= b ); };
long      OpEqual( long a, long b ){ return( a == b ); };
long   OpNotEqual( long a, long b ){ return( a != b ); };
long   OpLocicAnd( long a, long b ){ return( a && b ); };
long    OpLogicOr( long a, long b ){ return( a || b ); };
long     OpBitNot( long a, long b ){ return( ~a ); };
long   OpLogicNot( long a, long b ){ return( !a ); };
long OpUnaryMinus( long a, long b ){ return( -a ); };
long  OpUnaryPlus( long a, long b ){ return( a ); };

/// operators //////////////////////////////////////////////

OPERATOR
 operator[]={ { "",   OpError,      EXP_END },
                                          // single character FIRST !
              { ")",  OpError,      EXP_PAREN_R },
              { "(",  OpError,      EXP_PAREN_L },
              { "!",  OpLogicNot,   EXP_UNARY_NOT },
              { "~",  OpBitNot,     EXP_UNARY_NEG },
              { "+",  OpUnaryPlus,  EXP_UNARY_PLUS },      // unary operation
              { "+",  OpAdd,        EXP_ADDITIVE_PLUS },    // binary operation
              { "+",  OpError,      EXP_PLUS },              // token
              { "-",  OpUnaryMinus, EXP_UNARY_MINUS },     // unary operation
              { "-",  OpSub,        EXP_ADDITIVE_MINUS },   // binary operation
              { "-",  OpError,      EXP_MINUS },             // token
              { "*",  OpMulti,      EXP_MULTI_MUL },
              { "/",  OpDiv,        EXP_MULTI_DIV },
              { "%",  OpModulus,    EXP_MULTI_MODULUS },
              { "<",  OpLess,       EXP_RELATION_LESS },
              { ">",  OpGreat,      EXP_RELATION_GREAT },
              { "&",  OpBitAnd,     EXP_BITAND },
              { "^",  OpBitXor,     EXP_BITXOR },
              { "|",  OpBitOr,      EXP_BITOR },
                                          // double characters
              { "<<", OpShiftLeft,  EXP_SHIFT_L },
              { ">>", OpShiftRight, EXP_SHIFT_R },
              { "<=", OpLessEq,     EXP_RELATION_LESSEQ },
              { ">=", OpGreatEq,    EXP_RELATION_GREATEQ },
              { "==", OpEqual,      EXP_EQUAL_EQUAL },
              { "!=", OpNotEqual,   EXP_EQUAL_NOTEQUAL },
              { "&&", OpLocicAnd,   EXP_LOGICAND },
              { "||", OpLogicOr,    EXP_LOGICOR } };

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
 Calculator( BYTE *p, long *result, int *errPos, BYTE **errMsg )
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

BYTE
 *Expression( BYTE *p )
{
 return( ExpLogicOr( p ) );            // start at lower priority
}

////////////////////////////////////////////////////////////

BYTE  *ExpLogicOr( BYTE *p ){ return( Exp( p, ExpLogicAnd, EXP_LOGICOR ) ); }
BYTE *ExpLogicAnd( BYTE *p ){ return( Exp( p, ExpBitOr,    EXP_LOGICAND ) ); }
BYTE    *ExpBitOr( BYTE *p ){ return( Exp( p, ExpBitXor,   EXP_BITOR ) ); }
BYTE   *ExpBitXor( BYTE *p ){ return( Exp( p, ExpBitAnd,   EXP_BITXOR ) ); }
BYTE   *ExpBitAnd( BYTE *p ){ return( Exp( p, ExpEqual,    EXP_BITAND ) ); }
BYTE    *ExpEqual( BYTE *p ){ return( Exp( p, ExpRelat,    EXP_EQUAL ) ); }
BYTE    *ExpRelat( BYTE *p ){ return( Exp( p, ExpShift,    EXP_RELATION ) ); }
BYTE    *ExpShift( BYTE *p ){ return( Exp( p, ExpAdding,   EXP_SHIFT ) ); }
BYTE   *ExpAdding( BYTE *p ){ return( Exp( p, ExpMulti,    EXP_ADDITIVE ) ); }
BYTE    *ExpMulti( BYTE *p ){ return( Exp( p, ExpOperand,  EXP_MULTI ) ); }

////////////////////////////////////////////////////////////

BYTE
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

BYTE
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

BYTE
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

BYTE
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

int
 TokenIndex( BYTE *p )
{
 int
   i=sizeof( operator )/sizeof( OPERATOR );

 while( --i > 0 && memcmp( operator[ i ].op, p, strlen( operator[ i ].op ) ) )
   ;
 return( i );
}

////////////////////////////////////////////////////////////

int
 OperatorIndex( WORD type )
{
 int
   i=sizeof( operator )/sizeof( OPERATOR );

 while( --i > 0 && operator[ i ].token != type )
   ;
 return( i );
}

////////////////////////////////////////////////////////////

BYTE
 *SkipSpace( BYTE *p )
{
 while( isspace( *p ) )
   p++;
 return( p );
}

////////////////////////////////////////////////////////////

BYTE
 *Text2Dword( BYTE *s, DWORD *val, int base )
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
     *val=strtoul( s+prefix, &s, base );
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

BYTE
 Text2DwordLastChar( BYTE *s )
{
 if( *s == '-' || *s == '+' )
   s++;
 while( isxdigit( *s ) )
   s++;
 return( *s );
}

////////////////////////////////////////////////////////////

BYTE
 *Str2Dword( BYTE *s, long *a )
{
 *a=0L;
 while( s && *++s && *s != '"' ){
   if( *a & 0xFF000000L )
     s=NULL;
    else
     *a=( *a << 8 ) | ( ( *s == '\\' )? *++s : *s );
 }
 if( s && *s++ != '"' )
   s=NULL;
 return( s );
}

/// End of module //////////////////////////////////////////
