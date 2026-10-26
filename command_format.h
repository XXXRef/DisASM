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

typedef struct {
	char scale;
	char index;
	char base;
	unsigned char sib;
} SIB;

typedef struct {
	char mod;
	char reg;
	char rm;
	unsigned char modrm;// HEX view of modrm
} MODRM;

typedef struct {
		unsigned bit0:1;//TODO: implement as bit field?
		unsigned bit1:1;
		unsigned bit2:1;
		unsigned bit3:1;
		unsigned bit4:1;
		unsigned bit5:1;
		unsigned bit6:1;
		unsigned bit7:1;
	}SByte;

typedef union{
	char byte;
	SByte ubyte;
} UByte;

typedef struct {
	unsigned char bytenum;
	unsigned char*hex_bytes;//array of command bytes
	char*bytes;//binary view
} OPCODE;

typedef struct {
	unsigned char prefixcount;//number of prefixes
	unsigned char*p;//pointer to prefix array
	
	unsigned char lnr;//lock and repeat prefixes-f0, f2, f3
	unsigned char segr;//segment redefinition prefixes-2e, 36,3e,26,64,65
	unsigned char opr;//operand size redefenition prefix-66
	unsigned char sr;//address size redefenition prefix-67
} PREFIX;

typedef struct {
	char w;
	char s;
	char d;
	char reg;
	char tttn;
	char eee;//special purpose registers
	char uuu;//sreg3 (3 bits)
	char ff;//sreg2 (2 bits)
}SPECIALFIELDS;

typedef struct {
	PREFIX prefix;
	OPCODE opcode;
	SPECIALFIELDS sf;
	MODRM modrm;
	SIB sib;
	char*com_text;
	char**parameters;
	char*par;
	int par_count;
} SCommand;
