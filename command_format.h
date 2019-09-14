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

void SCommandInit(SCommand*command){
	int i=0;
	
	command->prefix.prefixcount=0;
	command->prefix.p=NULL;
	command->prefix.lnr=255; //TODO values in hex representation
	command->prefix.segr=255;
	command->prefix.opr=255;
	command->prefix.sr=255;
	
	command->opcode.bytenum=0;
	command->opcode.bytes=(char*)malloc(25); //24 bits + 0
	for(;i<25;i++){
		command->opcode.bytes[i]=0;
	}
	
	command->opcode.hex_bytes=(char*)malloc(3);
	
	command->sf.w=-128;
	command->sf.s=-128;
	command->sf.d=-128;
	command->sf.uuu=-128;
	command->sf.ff=-128;
	command->sf.reg=-128;
	command->sf.eee=-128;
	command->sf.tttn=-128;
	
	command->modrm.mod=-128;
	command->modrm.reg=-128;
	command->modrm.rm=-128;
	
	command->sib.scale=-128;
	command->sib.index=-128;
	command->sib.base=-128;
	command->sib.sib=255;
	
	command->com_text=NULL;
	command->parameters=NULL;
	command->par=NULL;
	command->par_count=0;
	return;
}
