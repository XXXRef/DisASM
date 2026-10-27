//@                                                                                               @\\ 
//@           _             _         _            _                  _            _   _          @\\ 
//@          /\ \          /\ \      / /\         / /\               / /\         /\_\/\_\ _      @\\ 
//@         /  \ \____     \ \ \    / /  \       / /  \             / /  \       / / / / //\_\    @\\ 
//@        / /\ \_____\    /\ \_\  / / /\ \__   / / /\ \           / / /\ \__   /\ \/ \ \/ / /    @\\ 
//@       / / /\/___  /   / /\/_/ / / /\ \___\ / / /\ \ \         / / /\ \___\ /  \____\__/ /     @\\ 
//@      / / /   / / /   / / /    \ \ \ \/___// / /  \ \ \        \ \ \ \/___// /\/________/      @\\ 
//@     / / /   / / /   / / /      \ \ \     / / /___/ /\ \        \ \ \     / / /\/_// / /       @\\ 
//@    / / /   / / /   / / /   _    \ \ \   / / /_____/ /\ \   _    \ \ \   / / /    / / /        @\\ 
//@    \ \ \__/ / /___/ / /__ /_/\__/ / /  / /_________/\ \ \ /_/\__/ / /  / / /    / / /         @\\ 
//@     \ \___\/ //\__\/_/___\\ \/___/ /  / / /_       __\ \_\\ \/___/ /   \/_/    / / /          @\\ 
//@      \/_____/ \/_________/ \_____\/   \_\___\     /____/_/ \_____\/            \/_/           @\\ 
//@                                                                                               @\\ 
//@                                                                                               @\\ 
//@        DisASM                                                                                 @\\ 
//@        @ Vlad Salnikov (XXXRef), 2013                                                         @\\ 
//@        xxxref.com                                                                             @\\ 
//@                                                                                               @\\ 

#include <stdio.h>

//TODO change char -> TYPE_BYTE

typedef struct
{
	char m_nScale;
	char m_nIndex;
	char m_nBase;
	unsigned char m_nSib;
} SIB;

typedef struct
{
	char m_nMod;
	char m_nReg;
	char m_nRm;
	unsigned char m_nModrm;// HEX view of modrm
} MODRM;

typedef struct
{
	unsigned m_nBit0:1;//TODO: implement as bit field?
	unsigned m_nBit1:1;
	unsigned m_nBit2:1;
	unsigned m_nBit3:1;
	unsigned m_nBit4:1;
	unsigned m_nBit5:1;
	unsigned m_nBit6:1;
	unsigned m_nBit7:1;
} SByte;

typedef union
{
	char m_nByte;
	SByte m_objUByte;
} UByte;

typedef struct
{
	unsigned char m_nByteNum;
	unsigned char* m_pHexBytes;//array of command bytes
	char* m_strBits;//binary view
} OPCODE;

typedef struct
{
	unsigned char m_nPrefixCount;//number of prefixes
	unsigned char* m_pBytes;//pointer to prefix array
	
	unsigned char m_nLnr;//lock and repeat prefixes-f0, f2, f3
	unsigned char m_nSegr;//segment redefinition prefixes-2e, 36,3e,26,64,65
	unsigned char m_nOpr;//operand size redefenition prefix-66
	unsigned char m_nSr;//address size redefenition prefix-67
} PREFIX;

typedef struct
{
	char m_nW;
	char m_nS;
	char m_nD;
	char m_nReg;
	char m_nTttn;
	char m_nEee;//special purpose registers
	char m_nUuu;//sreg3 (3 bits)
	char m_nFf;//sreg2 (2 bits)
} SPECIALFIELDS;

typedef struct
{
	PREFIX m_objPrefix;
	OPCODE m_objOpcode;
	SPECIALFIELDS m_objSpecialFields;
	MODRM m_objModrm;
	SIB m_objSib;
	char* m_strCommandText;
	char** m_pParameters;
	char* m_strParameters;
	int m_nParameterCount;
} SCommand;
