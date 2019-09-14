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
#include <windows.h>

#include "command_format.h"

#define MAX_COMMAND_LEN 15

#define RET_SUCCESS 0
#define RET_ERROR 1
#define RET_FILE_END 2

#define LINE_LEN 62
#define OPCODE_FIELD_OFFSET 0
#define EXT_FIELD_OFFSET 33
#define MODRM_FIELD_OFFSET 25
#define DESCRIPTION_FIELD_OFFSET 40

//====================================================================================================
char* SByte2Char(UByte*bp){
	char*mp;
	mp=(char*)malloc(9);//mem allocating
	mp[0]=(char)bp->ubyte.bit7+'0';
	mp[1]=(char)bp->ubyte.bit6+'0';
	mp[2]=(char)bp->ubyte.bit5+'0';
	mp[3]=(char)bp->ubyte.bit4+'0';
	mp[4]=(char)bp->ubyte.bit3+'0';
	mp[5]=(char)bp->ubyte.bit2+'0';
	mp[6]=(char)bp->ubyte.bit1+'0';
	mp[7]=(char)bp->ubyte.bit0+'0';
	mp[8]=0;
	return mp;
}

//====================================================================================================
int FindCommand(FILE*f,char*p,char*modrm){
	int text_counter=0,opcode_counter=0;
	char buf,buf2;
	while(1){
		fseek(f,text_counter,SEEK_SET);
		if(feof(f)){
			return -1;
		}
		while(1){
			fread(&buf,1,1,f);
			//printf("\tbuf:%c text_counter:%d\n",buf,text_counter);
			//getch();
			if(p[opcode_counter]==0){
				if(buf!=' ' && buf!='|'){
					//printf("\tNOT FOUND 1\n");
					opcode_counter=0;
					break;
				}
				
				
				//now modrm testing
				//printf("\t\tStarting to tezt reg field:\n");
				fseek(f,text_counter+MODRM_FIELD_OFFSET,SEEK_SET);//check is modrm present
				fread(&buf,1,1,f);
				if(buf==' '){//modrm isnt here
					return text_counter;
				}
				
				if(buf=='m'){//md
					if((modrm[0]=='1')&&(modrm[1]=='1')){
						opcode_counter=0;
						break;
					}
				}else{//11
					if(buf==modrm[0]){
						fread(&buf,1,1,f);
						if(buf!=modrm[1]){
							opcode_counter=0;
							break;
						}
					}else{
						opcode_counter=0;
						break;
					}
				}				
				
				fseek(f,text_counter+MODRM_FIELD_OFFSET+3,SEEK_SET);//+3 for reg/opcode field
				fread(&buf,1,1,f);
				
				if(buf!='0' && buf!='1'){
					//printf("FindCommand:2\n");
					return text_counter;
				}
				
				if(modrm[2]!=buf){
					opcode_counter=0;
					//printf("\tNOT FOUND -2\n");
					break;
				}
				fread(&buf,1,1,f);
				if(modrm[3]!=buf){
					opcode_counter=0;
					//printf("\tNOT FOUND 3\n");
					break;
				}
				fread(&buf,1,1,f);
				if(modrm[4]!=buf){
					opcode_counter=0;
					//printf("\tNOT FOUND 4\n");
					break;
				}
				//printf("FindCommand:3\n");
				return text_counter;
			}
			
			if((buf!=p[opcode_counter]) && (buf=='0' || buf=='1' || buf==' ')){
				//printf("\tNOT FOUND 5 opcode_counter:%d text_counter=%d\n\t\tbuf=%c p=%c\n",opcode_counter,text_counter,buf,p[opcode_counter]);
				opcode_counter=0;
				break;
			}
			
			if(buf=='f'){//pop s - push s processing
				fseek(f,ftell(f)+3,SEEK_SET);//0-push,1-pop
				fread(&buf,1,1,f);
				if(buf=='1'){//pop
					if(((p[opcode_counter]-'0')==0)&&((p[opcode_counter+1]-'0')==1)){//01-cs
						opcode_counter=0;
						break;
					}
				}
				fseek(f,ftell(f)-3,SEEK_SET);
				opcode_counter++;
			}
			
			if(buf=='u'){//uuu-sreg3 //pop s
				buf2=(p[opcode_counter]-'0')*4+(p[opcode_counter+1]-'0')*2+p[opcode_counter+2]-'0';
				if((buf2!=4)&&(buf2!=5)){//buf2!=100(fs) & buf2!=101(gs)
					opcode_counter=0;
					break;
				}
				fread(&buf,1,1,f);
				fread(&buf,1,1,f);
				opcode_counter+=2;
			}
			
			
			opcode_counter++;
		}
		text_counter+=LINE_LEN;
		while(1){//comments
			fseek(f,text_counter,SEEK_SET);
			if(fread(&buf,1,1,f)==0){//file end
				return -1;
			}
			
			//printf("\t2) buf:%c text_counter:%d\n",buf,text_counter);
			if(buf!=';'){
				// printf("\tbrokenhearted\n");
				break;	
			}
			text_counter+=LINE_LEN;
		}
	}
}

//====================================================================================================
int ParsePrefixes(SCommand*cp,const unsigned char*mp,int count){
	char buf=0;
	int bytecounter=0;
	while(bytecounter<count){
		//buf is used as flag that we can exit from while
		switch(mp[bytecounter]){
			//1 group
		case 0x66://redefinition of operand size
			cp->prefix.opr=0x66;//setting flag
			break;	
			
			//2 group			
		case 0x67://redefinition of address size
			cp->prefix.sr=0x67;//setting flag
			break;
			
			//3 group			
		case 0xF0://LOCK
		case 0xF2://REPNZ
		case 0xF3://REP
			cp->prefix.lnr=mp[bytecounter];//setting flag
			break;
			
			//4 group			
		case 0x2E: //CS
		case 0x36://SS
		case 0x3E: //DS
		case 0x26://ES
		case 0x64://FS
		case 0x65://GS
			cp->prefix.segr=mp[bytecounter];//setting flag
			break;
			
		default:
			buf=1;
		}
		if(buf==1){//exit
			break;
		}
		cp->prefix.prefixcount++;
		cp->prefix.p=(char*)realloc(cp->prefix.p,cp->prefix.prefixcount);
		cp->prefix.p[cp->prefix.prefixcount-1]=mp[bytecounter];
		bytecounter++;
	}
	if(bytecounter==count){
		return RET_ERROR;
	}
	return RET_SUCCESS;
}

//====================================================================================================
void Print(const SCommand*cp){
	int i=0;
	printf("SCommand:\n");
	
	printf("PREFIX:\n");//prefix
	printf("\tprefixcount:%d\n",cp->prefix.prefixcount);
	printf("\tp:");
	while(i<cp->prefix.prefixcount){
		printf(" %x",cp->prefix.p[i]);
		i++;
	}
	printf("\n");
	printf("\tlnr:%x\n",cp->prefix.lnr);
	printf("\tsegr:%x\n",cp->prefix.segr);
	printf("\topr:%x\n",cp->prefix.opr);
	printf("\tsr:%x\n",cp->prefix.sr);
	
	printf("OPCODE:\n");//opcodes
	printf("\tbytenum:%d\n",cp->opcode.bytenum);
	printf("\thex_bytes:");
	i=0;
	while(i<cp->opcode.bytenum){
		printf(" %#X",cp->opcode.hex_bytes[i]);
		i++;
	}
	printf("\n");
	printf("\tbytes:%s\n",cp->opcode.bytes);
	
	printf("EXT:\n");
	printf("\tw=%d s=%d d=%d reg=%d eee=%d uuu=%d ff=%d tttn=%d\n",cp->sf.w,cp->sf.s,cp->sf.d,cp->sf.reg,cp->sf.eee,cp->sf.uuu,cp->sf.ff,cp->sf.tttn);
	
	printf("MODRM:\n");
	printf("\tmodrm:%#X mod:%d reg:%d rm:%d\n",cp->modrm.modrm,cp->modrm.mod,cp->modrm.reg,cp->modrm.rm);
	printf("Command:\n");
	printf("\tcom_text:%s\n",cp->com_text);
	printf("\tpar:%s parcount:%d\n",cp->par,cp->par_count);
	i=0;
	printf("\tparameters:\n");
	while(i<cp->par_count){
		printf("\t\t%d: %s\n",i,cp->parameters[i]);
		i++;
	}
	printf("\nRESULT: %s",cp->com_text);//printing result
	if(cp->par_count!=0){
		printf(" %s",cp->parameters[0]);
		i=1;
		while(i<cp->par_count){
			printf(", %s",cp->parameters[i]);	
			i++;
		}
	}
	printf("\n");
	
}

//====================================================================================================
int GetCommand(SCommand*cp,FILE*hFile){
	UByte temp_byte;
	FILE*hFileOpcodes;
	int bytecounter=0,temp_int,ext_len=0,counter=0,com_len,com_len1;
	unsigned char*mp,*temp_char,*temp_char1,*temp_char2,*temp_char3;
	unsigned char buf,buf2;
	
	//printf("Begin GetCommand:\n");
	SCommandInit(cp);

	if((hFileOpcodes=fopen("res/opcodes.ttt","rb"))==0){
		printf("\tError: cant open \"opcodes.ttt\"\n");
		return RET_ERROR;
	}
	mp=(unsigned char*)malloc(MAX_COMMAND_LEN);//mem allocating
	com_len1=fread(mp,1,MAX_COMMAND_LEN,hFile);//reading data block, !file pointer+=MAX_COMMAND_LEN!
	
	if(com_len1==0){//file end
		return RET_FILE_END;
	}
	
	//printf("\"prefixes.ttt\" opened\n");
	/*------------------------------------------------------------prefix parsing------------------------------------------------------*/
	if(ParsePrefixes(cp,mp,com_len1)==RET_ERROR){
		printf("\tError: invalid command\n");
		return RET_ERROR;
	}
	bytecounter+=cp->prefix.prefixcount;
	/*------------------------------------------------------------prefix parsing------------------------------------------------------*/	
	
	/*------------------------------------------------------------command finding-----------------------------------------------------*/	
#ifdef _DEBUG
	printf("\tCommand finding:\n");
#endif
	
	buf=0;//like current command byte counter
	while(buf<3){//coz buf=0
		temp_byte.byte=mp[bytecounter];
		temp_char=SByte2Char(&temp_byte);
		strcpy(cp->opcode.bytes+buf*8,temp_char);
		cp->opcode.hex_bytes[buf]=mp[bytecounter];
		cp->opcode.bytenum++;
		free(temp_char);
		bytecounter++;
		temp_byte.byte=mp[bytecounter];//possible modrm
		temp_char=SByte2Char(&temp_byte);
#ifdef _DEBUG
		printf("\t\tPossible opcode:%s possible modrm:%s\n",cp->opcode.bytes,temp_char);
#endif
		if((temp_int=FindCommand(hFileOpcodes,cp->opcode.bytes,temp_char))!=-1){//command found
			break;//we can exit, buf<4
		}
		buf++;
	}
	
	if(buf==3){//command not found
		printf("\tError:command not found!\n");
		fclose(hFileOpcodes);
		return RET_ERROR;
	}
	//now in temp_int command position 
#ifdef _DEBUG
	printf("\tCommand position:%d\n",temp_int);
#endif
	/*------------------------------------------------------------command finding-----------------------------------------------------*/	
	
	/*------------------------------------------------------------ext parsing---------------------------------------------------------*/	

	fseek(hFileOpcodes,temp_int,SEEK_SET);//fp on command field
	ext_len=0;
	while(1){
		fread(&buf,1,1,hFileOpcodes);
		if(buf==' '){//cell end
			break;
		}
		switch(buf){
		case 's'://s
			cp->sf.s=cp->opcode.bytes[ext_len]-'0';
			break;
			
		case 'w'://w
			cp->sf.w=cp->opcode.bytes[ext_len]-'0';
			break;
			
		case 'd'://d
			cp->sf.d=cp->opcode.bytes[ext_len]-'0';
			break;
			
		case 'r'://reg
			cp->sf.reg=(cp->opcode.bytes[ext_len]-'0')*4+(cp->opcode.bytes[ext_len+1]-'0')*2+(cp->opcode.bytes[ext_len+2]-'0');
			ext_len+=2;
			fread(&buf,1,1,hFileOpcodes);
			fread(&buf,1,1,hFileOpcodes);
			break;
			
		case 't'://tttn
			cp->sf.tttn=(cp->opcode.bytes[ext_len]-'0')*8+(cp->opcode.bytes[ext_len+1]-'0')*4+(cp->opcode.bytes[ext_len+2]-'0')*2+(cp->opcode.bytes[ext_len+3]-'0');
			ext_len+=3;
			fread(&buf,1,1,hFileOpcodes);
			fread(&buf,1,1,hFileOpcodes);
			fread(&buf,1,1,hFileOpcodes);
			break;
		case 'e'://eee special purpose registers (cr, dr)
			cp->sf.eee=(cp->opcode.bytes[ext_len]-'0')*4+(cp->opcode.bytes[ext_len+1]-'0')*2+(cp->opcode.bytes[ext_len+2]-'0');
			ext_len+=2;
			fread(&buf,1,1,hFileOpcodes);
			fread(&buf,1,1,hFileOpcodes);
			break;
		case 'f'://ff-sreg2(2 bits) 
			cp->sf.ff=(cp->opcode.bytes[ext_len]-'0')*2+(cp->opcode.bytes[ext_len+1]-'0');
			ext_len+=1;
			fread(&buf,1,1,hFileOpcodes);
			break;
		case 'u'://uuu-sreg3(3 bits)
			cp->sf.uuu=(cp->opcode.bytes[ext_len]-'0')*4+(cp->opcode.bytes[ext_len+1]-'0')*2+(cp->opcode.bytes[ext_len+2]-'0');
			ext_len+=2;
			fread(&buf,1,1,hFileOpcodes);
			fread(&buf,1,1,hFileOpcodes);
			break;
			/*default:
			printf("\tError:cant find ext. buf=%c\n",buf);
			getch();
			return -1;*/	
		}
		ext_len++;
	}
	/*------------------------------------------------------------ext parsing---------------------------------------------------------*/	
	/*----------------------------------------------------------modrm parsing---------------------------------------------------------*/	
	fseek(hFileOpcodes,temp_int+MODRM_FIELD_OFFSET,SEEK_SET);//check modrm is present
	fread(&buf,1,1,hFileOpcodes);
	if(buf!=' '){//modrm is here
		cp->modrm.modrm=mp[bytecounter];
		cp->modrm.mod=(temp_char[0]-'0')*2+(temp_char[1]-'0');
		cp->modrm.reg=(temp_char[2]-'0')*4+(temp_char[3]-'0')*2+(temp_char[4]-'0');
		cp->modrm.rm=(temp_char[5]-'0')*4+(temp_char[6]-'0')*2+(temp_char[7]-'0');
		bytecounter++;
	}
	free(temp_char);//temp_char:possible modrm
#ifdef _DEBUG
	printf("\tMODRM:mod:%d reg:%d rm:%d\n",cp->modrm.mod,cp->modrm.reg,cp->modrm.rm);
#endif
	/*----------------------------------------------------------modrm parsing---------------------------------------------------------*/

	/*--------------------------------------------------------command parsing---------------------------------------------------------*/
	fseek(hFileOpcodes,temp_int+DESCRIPTION_FIELD_OFFSET,SEEK_SET);
	//printf("\tfseek=%d temp_int+DESCRIPTION_FIELD_OFFSET=%d\n",ftell(hFileOpcodes),temp_int+DESCRIPTION_FIELD_OFFSET);
	ext_len=0;
	while(1){//readindg command
		fread(&buf,1,1,hFileOpcodes);
		if(buf==' '){
			break;
		}
		cp->com_text=(char*)realloc( cp->com_text,ext_len+2);
		cp->com_text[ext_len]=buf;
		ext_len++;
	}
	cp->com_text[ext_len]=0;
#ifdef _DEBUG
	printf("\tcom_text:%s\n",cp->com_text);
#endif
	/*--------------------------------------------------------command parsing---------------------------------------------------------*/

	/*--------------------------------------------------------operand parsing---------------------------------------------------------*/
	//operands reading------------
	ext_len=0;
	while(1){
		cp->par=(char*)realloc(cp->par,ext_len+2);
		fread(&buf,1,1,hFileOpcodes);
		if(buf==' '){// no operands | file end
			break;
		}
		
		cp->par[ext_len]=buf;
		cp->par_count++;
		ext_len++;
	}	
	cp->par[ext_len]=0;
	
	//exchanging if d==0---------------------	
	if((cp->sf.d==0) && (cp->par_count>=2)){//exchanging ('d' field) if not immediate as 1st par
		if(cp->par[1]!='i'){
			buf=cp->par[1];
			cp->par[1]=cp->par[0];
			cp->par[0]=buf;
		}
	}
	
	cp->parameters=(char**)malloc(sizeof(char*)*(cp->par_count));//memory allocating
	ext_len=0;
	while(ext_len<cp->par_count){
		cp->parameters[ext_len]=(char*)malloc(40);//[eax+ebx*1+0FFFFFFFF],0
		ext_len++;
	}
	//printf("DEBUG");	
	//-----------------------------------------------operands parsing----------------------------------------------------------------------------------------------------------------------------------------------------------	
	while(counter<cp->par_count){
		switch(cp->par[counter]){
			
		case 'r'://reg1
			if(cp->sf.reg!=-128){//reg (reg field is always last 3 bytes of opcode)
				ext_len=cp->sf.reg;
				//printf("\t\tpos in ret:%d temp_char:%s\n",Find(hFileRet,temp_char1),temp_char1);
			}else{//search in mod/opcode section
				fseek(hFileOpcodes,temp_int+MODRM_FIELD_OFFSET+3,SEEK_SET);//fp on mod/opcode section
				fread(&buf,1,1,hFileOpcodes);
				if(buf=='r'){
					//check if reg1
					fseek(hFileOpcodes,temp_int+MODRM_FIELD_OFFSET+6,SEEK_SET);
					fread(&buf,1,1,hFileOpcodes);
					if(buf=='1' || buf==' '){//reg1 | reg
						ext_len=cp->modrm.reg;
					}else{//reg1 in rm section
						ext_len=cp->modrm.rm;
					}
				}else{//reg1 in rm section
					ext_len=cp->modrm.rm;
				}
			}
			//in ext_len number of register
			switch(ext_len){
			case 0://al ax eax
				if(cp->sf.w==0){
					cp->parameters[counter][0]='a';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='a';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='a';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;
			case 1://ecx
				if(cp->sf.w==0){
					cp->parameters[counter][0]='c';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='c';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='c';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;
				
				
			case 2://edx
				if(cp->sf.w==0){
					cp->parameters[counter][0]='d';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='d';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='d';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;

			case 3://ebx
				if(cp->sf.w==0){
					cp->parameters[counter][0]='b';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='b';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='b';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;

			case 4://esp
				if(cp->sf.w==0){
					cp->parameters[counter][0]='s';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='s';
						cp->parameters[counter][1]='p';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='s';
						cp->parameters[counter][2]='p';
						cp->parameters[counter][3]=0;
					}
				}
				break;						
				
			case 5://ebp
				if(cp->sf.w==0){
					cp->parameters[counter][0]='b';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='b';
						cp->parameters[counter][1]='p';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='b';
						cp->parameters[counter][2]='p';
						cp->parameters[counter][3]=0;
					}
				}
				break;
				
			case 6://esi
				if(cp->sf.w==0){
					cp->parameters[counter][0]='e';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='s';
						cp->parameters[counter][1]='i';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='s';
						cp->parameters[counter][2]='i';
						cp->parameters[counter][3]=0;
					}
				}
				break;
				
			case 7://edi
				if(cp->sf.w==0){
					cp->parameters[counter][0]='d';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='d';
						cp->parameters[counter][1]='i';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='d';
						cp->parameters[counter][2]='i';
						cp->parameters[counter][3]=0;
					}
				}
				break;
			}
			break;
			
			//------------------------------------reg2----------------------------------------------------------------------			
		case 'p'://reg2
			fseek(hFileOpcodes,temp_int+MODRM_FIELD_OFFSET+3,SEEK_SET);//fp on reg/opcode section
			fread(&buf,1,1,hFileOpcodes);
			if(buf=='r'){//check if reg2
				fseek(hFileOpcodes,temp_int+MODRM_FIELD_OFFSET+6,SEEK_SET);
				fread(&buf,1,1,hFileOpcodes);
				if(buf=='2'){//reg2
					ext_len=cp->modrm.reg;
				}else{//reg2 in rm section
					ext_len=cp->modrm.rm;
				}
			}else{//reg2 in rm section
				ext_len=cp->modrm.rm;
			}	
			//in ext_len number of register
			switch(ext_len){
			case 0://al ax eax
				if(cp->sf.w==0){
					cp->parameters[counter][0]='a';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='a';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='a';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;
			case 1://ecx
				if(cp->sf.w==0){
					cp->parameters[counter][0]='c';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='c';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='c';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;
				
				
			case 2://edx
				if(cp->sf.w==0){
					cp->parameters[counter][0]='d';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='d';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='d';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;

			case 3://ebx
				if(cp->sf.w==0){
					cp->parameters[counter][0]='b';
					cp->parameters[counter][1]='l';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='b';
						cp->parameters[counter][1]='x';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='b';
						cp->parameters[counter][2]='x';
						cp->parameters[counter][3]=0;
					}
				}
				break;

			case 4://esp
				if(cp->sf.w==0){
					cp->parameters[counter][0]='s';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='s';
						cp->parameters[counter][1]='p';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='s';
						cp->parameters[counter][2]='p';
						cp->parameters[counter][3]=0;
					}
				}
				break;						
				
			case 5://ebp
				if(cp->sf.w==0){
					cp->parameters[counter][0]='b';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='b';
						cp->parameters[counter][1]='p';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='b';
						cp->parameters[counter][2]='p';
						cp->parameters[counter][3]=0;
					}
				}
				break;
				
			case 6://esi
				if(cp->sf.w==0){
					cp->parameters[counter][0]='e';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='s';
						cp->parameters[counter][1]='i';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='s';
						cp->parameters[counter][2]='i';
						cp->parameters[counter][3]=0;
					}
				}
				break;
				
			case 7://edi
				if(cp->sf.w==0){
					cp->parameters[counter][0]='d';
					cp->parameters[counter][1]='d';
					cp->parameters[counter][2]=0;
				}else{
					if(cp->prefix.opr==0x66){//66h - operand-size prefix
						cp->parameters[counter][0]='d';
						cp->parameters[counter][1]='i';
						cp->parameters[counter][2]=0;
						break;
					}else{
						cp->parameters[counter][0]='e';
						cp->parameters[counter][1]='d';
						cp->parameters[counter][2]='i';
						cp->parameters[counter][3]=0;
					}
				}
				break;
			}
			break;
			
			//-------------------------------------------------------------sreg----------------------------------------------			
		case 's'://sreg
			if(cp->sf.ff!=-128){
				ext_len=cp->sf.ff;
			}else{
				if(cp->sf.uuu!=-128){
					ext_len=cp->sf.uuu;
				}else{
					fseek(hFileOpcodes,temp_int+MODRM_FIELD_OFFSET+3,SEEK_SET);//fp on mod/opcode section
					fread(&buf,1,1,hFileOpcodes);
					if(buf=='s'){
						ext_len=cp->modrm.reg;
						//printf("\t\tDEBUG\n");
					}else{//sreg in rm section
						ext_len=cp->modrm.rm;
						//printf("\t\tDEBUG\n");
					}
				}
			}
			//in ext_len number of register
			switch(ext_len){
			case 0://es
				cp->parameters[counter][0]='e';
				cp->parameters[counter][1]='s';
				cp->parameters[counter][2]=0;
				break;
				
			case 1://cs
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='s';
				cp->parameters[counter][2]=0;
				break;
				
			case 2://ss
				cp->parameters[counter][0]='s';
				cp->parameters[counter][1]='s';
				cp->parameters[counter][2]=0;
				break;

			case 3://ds
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='s';
				cp->parameters[counter][2]=0;
				break;

			case 4://fs
				cp->parameters[counter][0]='f';
				cp->parameters[counter][1]='s';
				cp->parameters[counter][2]=0;
				break;						
				
			case 5://gs
				cp->parameters[counter][0]='g';
				cp->parameters[counter][1]='s';
				cp->parameters[counter][2]=0;
				break;
				
			case 6://reserved1
				fclose(hFileOpcodes);
				free(mp);
				return RET_ERROR;//error
				break;
				
			case 7://reserved2
				return RET_ERROR;//error
				fclose(hFileOpcodes);
				free(mp);
				break;
			}
			break;
			
			//---------------------------------------------------- 1 ------------------------------------------------------			
		case '1':
			cp->parameters[counter][0]='1';
			cp->parameters[counter][1]=0;
			break;
			
			//-------------------------------------------------al ax eax---------------------------------------------------
		case 'a':
			if(cp->sf.w==0){//al
				cp->parameters[counter][0]='a';
				cp->parameters[counter][1]='l';
				cp->parameters[counter][2]=0;
			}else{
				if(cp->prefix.opr==0x66){//ax
					cp->parameters[counter][0]='a';
					cp->parameters[counter][1]='x';
					cp->parameters[counter][2]=0;
				}else{//eax
					cp->parameters[counter][0]='e';
					cp->parameters[counter][1]='a';
					cp->parameters[counter][2]='x';
					cp->parameters[counter][3]=0;
				}
			}
			break;
			//------------------------------------------------- cl ---------------------------------------------------			
		case 'b'://cl
			cp->parameters[counter][0]='c';
			cp->parameters[counter][1]='l';
			cp->parameters[counter][2]=0;
			break;
			
			//------------------------------------------------- dr0-7 ---------------------------------------------------
		case 'd'://dr0-7		
			if(cp->sf.eee!=-128){
				ext_len=cp->sf.eee;
			}else{
				fseek(hFileOpcodes,MODRM_FIELD_OFFSET+3,SEEK_SET);
				fread(&buf,1,1,hFileOpcodes);
				if(buf=='e'){//eee
					ext_len=cp->modrm.reg;
				}else{//in rm field
					ext_len=cp->modrm.rm;
				}
			}
			switch(ext_len){
			case 0://dr0
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='0';
				cp->parameters[counter][3]=0;
				break;
				
			case 1://dr1
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='1';
				cp->parameters[counter][3]=0;
				break;
				
			case 2://dr2
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='2';
				cp->parameters[counter][3]=0;
				break;

			case 3://dr3
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='3';
				cp->parameters[counter][3]=0;
				break;

			case 4://dr4
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='4';
				cp->parameters[counter][3]=0;
				break;						
				
			case 5://dr5
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='5';
				cp->parameters[counter][3]=0;
				break;
				
			case 6://dr6
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='6';
				cp->parameters[counter][3]=0;
				break;
				
			case 7://dr7
				cp->parameters[counter][0]='d';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='7';
				cp->parameters[counter][3]=0;
				break;
			}
			break;
			//------------------------------------------------- cr0-7 ---------------------------------------------------			
		case 'c'://cr0-7
#ifdef _DEBUG
			printf("\toperand1:cr0-7\n");
#endif
			if(cp->sf.eee!=-128){
				ext_len=cp->sf.eee;
			}else{
				fseek(hFileOpcodes,MODRM_FIELD_OFFSET+3,SEEK_SET);
				fread(&buf,1,1,hFileOpcodes);
				if(buf=='e'){//eee
					ext_len=cp->modrm.reg;
				}else{//in rm field
					ext_len=cp->modrm.rm;
				}
			}
			switch(ext_len){
			case 0://cr0
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='0';
				cp->parameters[counter][3]=0;
				break;
				
			case 1://cr1
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='1';
				cp->parameters[counter][3]=0;
				break;
				
			case 2://cr2
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='2';
				cp->parameters[counter][3]=0;
				break;

			case 3://cr3
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='3';
				cp->parameters[counter][3]=0;
				break;

			case 4://cr4
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='4';
				cp->parameters[counter][3]=0;
				break;						
				
			case 5://cr5
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='5';
				cp->parameters[counter][3]=0;
				break;
				
			case 6://cr6
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='6';
				cp->parameters[counter][3]=0;
				break;
				
			case 7://cr7
				cp->parameters[counter][0]='c';
				cp->parameters[counter][1]='r';
				cp->parameters[counter][2]='7';
				cp->parameters[counter][3]=0;
				break;
			}
			break;
			
			//------------------------------------------------- memory ---------------------------------------------------				
		case 'm'://0x67, 0x66, segment redefinition prefixes affects
			if(cp->sf.w==0){//1 byte operand 
				cp->parameters[counter][0]='B';
				cp->parameters[counter][1]='Y';
				cp->parameters[counter][2]='T';
				cp->parameters[counter][3]='E';
				com_len=4;
			}else{//2|4 byte operand 
				if(cp->prefix.opr==0x66){//0x66 -2byte
					cp->parameters[counter][0]='W';
					cp->parameters[counter][1]='O';
					cp->parameters[counter][2]='R';
					cp->parameters[counter][3]='D';
					com_len=4;
				}else{//no 0x66 -4byte
					cp->parameters[counter][0]='D';
					cp->parameters[counter][1]='W';
					cp->parameters[counter][2]='O';
					cp->parameters[counter][3]='R';
					cp->parameters[counter][4]='D';
					com_len=5;
				}
			}

			cp->parameters[counter][com_len]='_';
			cp->parameters[counter][com_len+1]='P';
			cp->parameters[counter][com_len+2]='T';
			cp->parameters[counter][com_len+3]='R';
			cp->parameters[counter][com_len+4]='_';
			com_len+=5;

			switch(cp->prefix.segr){
			case 0x2e: //cs
				cp->parameters[counter][com_len]='C';
				break;
			case 0x36://ss
				cp->parameters[counter][com_len]='S';
				
				break;
			case 0x26: //es
				cp->parameters[counter][com_len]='E';
				
				break;
			case 0x64: //fs
				cp->parameters[counter][com_len]='C';
				
				break;
			case 0x65: //gs
				cp->parameters[counter][com_len]='G';
				break;
			default: //ds
				cp->parameters[counter][com_len]='D';
				break;
			}

			cp->parameters[counter][com_len+1]='S';
			cp->parameters[counter][com_len+2]=':';
			cp->parameters[counter][com_len+3]='[';
			com_len+=3;

			if(cp->prefix.sr!=0x67){//0x67 not present 
				cp->parameters[counter][com_len+1]='e';
				switch(cp->modrm.rm){
				case 0://eax
					cp->parameters[counter][com_len+2]='a';
					cp->parameters[counter][com_len+3]='x';
					buf=4;
					break;
					
				case 1://ecx
					cp->parameters[counter][com_len+2]='c';
					cp->parameters[counter][com_len+3]='x';
					buf=4;
					break;
					
				case 2://edx
					cp->parameters[counter][com_len+2]='d';
					cp->parameters[counter][com_len+3]='x';
					buf=4;
					break;
					
				case 3://ebx
					cp->parameters[counter][com_len+2]='b';
					cp->parameters[counter][com_len+3]='x';
					buf=4;
					break;
					
				case 4://SIB
					cp->sib.scale=mp[bytecounter]/64;
					cp->sib.index=mp[bytecounter]/8-(mp[bytecounter]/64)*8;
					cp->sib.base=mp[bytecounter]%8;
					cp->sib.sib=mp[bytecounter];
					bytecounter++;//SIB parsed

					//cp->parameters[counter][1]='e';
					switch(cp->sib.base){
					case 0:
						cp->parameters[counter][com_len+2]='a';
						cp->parameters[counter][com_len+3]='x';
						break;
						
					case 1:
						cp->parameters[counter][com_len+2]='c';
						cp->parameters[counter][com_len+3]='x';
						break;
						
					case 2:
						cp->parameters[counter][com_len+2]='d';
						cp->parameters[counter][com_len+3]='x';
						break;
						
					case 3:
						cp->parameters[counter][com_len+2]='b';
						cp->parameters[counter][com_len+3]='x';
						break;
						
					case 4:
						cp->parameters[counter][com_len+2]='s';
						cp->parameters[counter][com_len+3]='p';
						break;
						
					case 5://[*]
						cp->parameters[counter][com_len+2]='b';
						cp->parameters[counter][com_len+3]='p';
						break;
						
					case 6:
						cp->parameters[counter][com_len+2]='s';
						cp->parameters[counter][com_len+3]='i';
						break;
						
					case 7:
						cp->parameters[counter][com_len+2]='d';
						cp->parameters[counter][com_len+3]='i';
						break;
					}
					
					cp->parameters[counter][com_len+4]='+';
					cp->parameters[counter][com_len+5]='e';
					
					switch(cp->sib.index){
					case 0:
						cp->parameters[counter][com_len+6]='a';
						cp->parameters[counter][com_len+7]='x';
						break;
						
					case 1:
						cp->parameters[counter][com_len+6]='c';
						cp->parameters[counter][com_len+7]='x';
						break;
						
					case 2:
						cp->parameters[counter][com_len+6]='d';
						cp->parameters[counter][com_len+7]='x';
						break;
						
					case 3:
						cp->parameters[counter][com_len+6]='b';
						cp->parameters[counter][com_len+7]='x';
						break;
						
					case 4:
						cp->parameters[counter][com_len+6]='s';
						cp->parameters[counter][com_len+7]='p';
						break;
						
					case 5://[*]
						cp->parameters[counter][com_len+6]='b';
						cp->parameters[counter][com_len+7]='p';
						break;
						
					case 6:
						cp->parameters[counter][com_len+6]='s';
						cp->parameters[counter][com_len+7]='i';
						break;
						
					case 7:
						cp->parameters[counter][com_len+6]='d';
						cp->parameters[counter][com_len+7]='i';
						break;
					}
					
					cp->parameters[counter][com_len+8]='*';
					ext_len=0;
					buf=1;
					
					while(ext_len<cp->sib.scale){
						buf*=2;
						ext_len++;
					}	
					cp->parameters[counter][com_len+9]=buf+'0';
					buf=10;
					break;
					
				case 5://can be displacement only
					if(cp->modrm.mod==0){
						ext_len=4;
						while(ext_len!=0){
							cp->parameters[counter][com_len+ext_len*2]=mp[bytecounter]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if((cp->parameters[counter][com_len+ext_len*2]-'0')>9){				
								cp->parameters[counter][com_len+ext_len*2]='A'+cp->parameters[counter][com_len+ext_len*2]-'0'-10;
							}
							cp->parameters[counter][com_len+ext_len*2-1]=mp[bytecounter]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if((cp->parameters[counter][com_len+ext_len*2-1]-'0')>9){
								cp->parameters[counter][com_len+ext_len*2-1]='A'+cp->parameters[counter][com_len+ext_len*2-1]-'0'-10;
							}
							bytecounter++;
							ext_len--;
						}
						buf=9;
					}else{
						cp->parameters[counter][com_len+2]='b';
						cp->parameters[counter][com_len+3]='p';
						buf=4;
					}
					break;
					
				case 6://esi
					//cp->parameters[counter][1]='e';
					cp->parameters[counter][com_len+2]='s';
					cp->parameters[counter][com_len+3]='i';
					buf=4;
					break;
					
				case 7://edi
					//cp->parameters[counter][1]='e';
					cp->parameters[counter][com_len+2]='d';
					cp->parameters[counter][com_len+3]='i';
					buf=4;
					break;	
				}
				ext_len=cp->modrm.mod*cp->modrm.mod;//in ext_len number of displacement bytes
				if(ext_len!=0){
					if((ext_len==1) && (mp[bytecounter]>0x7f)){//1 byte sign extended
						cp->parameters[counter][com_len+buf]='-';
						cp->parameters[counter][com_len+buf+1]=(0x100-mp[bytecounter])/16+'0';
						if((cp->parameters[counter][com_len+buf+1]-'0')>9){				
							cp->parameters[counter][com_len+buf+1]='A'+cp->parameters[counter][com_len+buf+1]-'0'-10;
						}
						cp->parameters[counter][com_len+buf+2]=(0x100-mp[bytecounter])%16+'0';
						if((cp->parameters[counter][com_len+buf+2]-'0')>9){				
							cp->parameters[counter][com_len+buf+2]='A'+cp->parameters[counter][com_len+buf+2]-'0'-10;
						}
					}else{
						cp->parameters[counter][com_len+buf]='+';
						while(ext_len!=0){
							cp->parameters[counter][com_len+buf+ext_len*2]=mp[bytecounter]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if((cp->parameters[counter][com_len+buf+ext_len*2]-'0')>9){				
								cp->parameters[counter][com_len+buf+ext_len*2]='A'+cp->parameters[counter][com_len+buf+ext_len*2]-'0'-10;
							}
							cp->parameters[counter][com_len+buf+ext_len*2-1]=mp[bytecounter]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if((cp->parameters[counter][com_len+buf+ext_len*2-1]-'0')>9){
								cp->parameters[counter][com_len+buf+ext_len*2-1]='A'+cp->parameters[counter][com_len+buf+ext_len*2-1]-'0'-10;
							}
							bytecounter++;
							ext_len--;
						}
					}
					buf=buf+cp->modrm.mod*cp->modrm.mod*2+1;
				}
				cp->parameters[counter][com_len+buf]=']';
				cp->parameters[counter][com_len+buf+1]=0;				
			}else{//0x67 present
				switch(cp->modrm.rm){
				case 0:
					cp->parameters[counter][com_len+1]='b';
					cp->parameters[counter][com_len+2]='x';
					cp->parameters[counter][com_len+3]='+';
					cp->parameters[counter][com_len+4]='s';
					cp->parameters[counter][com_len+5]='i';
					buf=6;
					break;
					
				case 1:
					cp->parameters[counter][com_len+1]='b';
					cp->parameters[counter][com_len+2]='x';
					cp->parameters[counter][com_len+3]='+';
					cp->parameters[counter][com_len+4]='d';
					cp->parameters[counter][com_len+5]='i';
					buf=6;
					break;
					
				case 2:
					cp->parameters[counter][com_len+1]='b';
					cp->parameters[counter][com_len+2]='p';
					cp->parameters[counter][com_len+3]='+';
					cp->parameters[counter][com_len+4]='s';
					cp->parameters[counter][com_len+5]='i';
					buf=6;
					break;
					
				case 3:
					cp->parameters[counter][com_len+1]='b';
					cp->parameters[counter][com_len+2]='p';
					cp->parameters[counter][com_len+3]='+';
					cp->parameters[counter][com_len+4]='d';
					cp->parameters[counter][com_len+5]='i';
					buf=6;
					break;
					
				case 4:
					cp->parameters[counter][com_len+1]='s';
					cp->parameters[counter][com_len+2]='i';
					buf=3;
					break;
					
				case 5:
					cp->parameters[counter][com_len+1]='d';
					cp->parameters[counter][com_len+2]='i';
					buf=3;
					break;
					
				case 6://can be disp16 only
					if(cp->modrm.mod==0){
						ext_len=2;
						while(ext_len!=0){
							cp->parameters[counter][com_len+ext_len*2]=mp[bytecounter]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if((cp->parameters[counter][com_len+ext_len*2]-'0')>9){				
								cp->parameters[counter][com_len+ext_len*2]='A'+cp->parameters[counter][com_len+ext_len*2]-'0'-10;
							}
							cp->parameters[counter][com_len+ext_len*2-1]=mp[bytecounter]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if((cp->parameters[counter][com_len+ext_len*2-1]-'0')>9){
								cp->parameters[counter][com_len+ext_len*2-1]='A'+cp->parameters[counter][com_len+ext_len*2-1]-'0'-10;
							}
							bytecounter++;
							ext_len--;
						}
						buf=5;
					}else{
						cp->parameters[counter][com_len+1]='b';
						cp->parameters[counter][com_len+2]='p';
						buf=3;
					}
					break;
					
				case 7:
					cp->parameters[counter][com_len+1]='b';
					cp->parameters[counter][com_len+2]='x';
					buf=3;
					break;
				}
				
				ext_len=cp->modrm.mod;//in ext_len number of displacement bytes
				if(ext_len!=0){
					
					if((ext_len==1) && (mp[bytecounter]>0x7f)){//1 byte sign extended
						cp->parameters[counter][com_len+buf]='-';
						cp->parameters[counter][com_len+buf+1]=(0x100-mp[bytecounter])/16+'0';
						if((cp->parameters[counter][com_len+buf+1]-'0')>9){				
							cp->parameters[counter][com_len+buf+1]='A'+cp->parameters[counter][com_len+buf+1]-'0'-10;
						}
						cp->parameters[counter][com_len+buf+2]=(0x100-mp[bytecounter])%16+'0';
						if((cp->parameters[counter][com_len+buf+2]-'0')>9){				
							cp->parameters[counter][com_len+buf+2]='A'+cp->parameters[counter][com_len+buf+2]-'0'-10;
						}
					}else{
						
						cp->parameters[counter][com_len+buf]='+';
						while(ext_len!=0){
							cp->parameters[counter][com_len+buf+ext_len*2]=mp[bytecounter]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if((cp->parameters[counter][com_len+buf+ext_len*2]-'0')>9){				
								cp->parameters[counter][com_len+buf+ext_len*2]='A'+cp->parameters[counter][com_len+buf+ext_len*2]-'0'-10;
							}
							cp->parameters[counter][com_len+buf+ext_len*2-1]=mp[bytecounter]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if((cp->parameters[counter][com_len+buf+ext_len*2-1]-'0')>9){
								cp->parameters[counter][com_len+buf+ext_len*2-1]='A'+cp->parameters[counter][com_len+buf+ext_len*2-1]-'0'-10;
							}
							bytecounter++;
							ext_len--;
						}
					}
					buf=buf+cp->modrm.mod*2+1;
				}
				cp->parameters[counter][com_len+buf]=']';
				cp->parameters[counter][com_len+buf+1]=0;
			}

			break;
			
			//-----------------------------------------------immediate--------------------------------------------------	
		case 'i'://immediate //0x66/,s,w (,d) affects
			ext_len=1;
			if(cp->sf.s==1){//1byte imm
				if(mp[bytecounter]>0x7F){//-
					cp->parameters[counter][0]='-';
					cp->parameters[counter][1]=(0x100-mp[bytecounter])/16+'0';
					cp->parameters[counter][2]=(0x100-mp[bytecounter])%16+'0';
					if((cp->parameters[counter][1]-'0')>9){
						cp->parameters[counter][1]='A'+cp->parameters[counter][1]-'0'-10;
					}
					if((cp->parameters[counter][2]-'0')>9){
						cp->parameters[counter][2]='A'+cp->parameters[counter][2]-'0'-10;
					}
					cp->parameters[counter][3]=0;
					bytecounter++;
					break;
				}
			}else{
				if(cp->sf.w==1){//2,4 byte imm
					if(cp->prefix.opr==0x66){//2 bytes imm
						ext_len =2;
					}else{//4 bytes imm
						ext_len=4;
					}
				}
			}

			cp->parameters[counter][ext_len*2]=0;

			while(ext_len!=0){
				cp->parameters[counter][ext_len*2-1]=mp[bytecounter]%16+'0';
				//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
				//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
				if((cp->parameters[counter][ext_len*2-1]-'0')>9){				
					cp->parameters[counter][ext_len*2-1]='A'+cp->parameters[counter][ext_len*2-1]-'0'-10;
				}
				cp->parameters[counter][ext_len*2-2]=mp[bytecounter]/16+'0';
				//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
				if((cp->parameters[counter][ext_len*2-2]-'0')>9){
					cp->parameters[counter][ext_len*2-2]='A'+cp->parameters[counter][ext_len*2-2]-'0'-10;
				}
				bytecounter++;
				ext_len--;
			}

			break;
			
			//8bit immediate----------------------------------------------------------------------------------------------------
		case '8':
			cp->parameters[counter][0]=mp[bytecounter]/16+'0';
			if((cp->parameters[counter][0]-'0')>9){
				cp->parameters[counter][0]='A'+cp->parameters[counter][0]-'0'-10;
			}
			cp->parameters[counter][1]=mp[bytecounter]%16+'0';
			if((cp->parameters[counter][1]-'0')>9){
				cp->parameters[counter][1]='A'+cp->parameters[counter][1]-'0'-10;
			}
			cp->parameters[counter][2]=0;
			bytecounter++;
			break;
			
			//16bit immediate----------------------------------------------------------------------------------------------------
		case '6':
			buf=2;
			while(buf!=0){//0,1
				buf--;
				cp->parameters[counter][buf*2]=mp[bytecounter]/16+'0';
				if((cp->parameters[counter][buf*2]-'0')>9){
					cp->parameters[counter][buf*2]='A'+cp->parameters[counter][0]-'0'-10;
				}
				cp->parameters[counter][buf*2+1]=mp[bytecounter]%16+'0';
				if((cp->parameters[counter][buf*2+1]-'0')>9){
					cp->parameters[counter][buf*2+1]='A'+cp->parameters[counter][buf*2+1]-'0'-10;
				}
				bytecounter++;
			}
			cp->parameters[counter][4]=0;
			break;
			
			//dx(ex. variable port)----------------------------------------------------------------------------------------------------
		case 'v':
			cp->parameters[counter][0]='d';
			cp->parameters[counter][1]='x';
			cp->parameters[counter][2]=0;
			break;
			//tttn-----------------------------------------------------------------------------------------------------------------------------------
		case 't'://tttn
			cp->par_count--;//tttn is modification, not parameter
			ext_len=0;
			while(cp->com_text[ext_len]!=0){
				ext_len++;
			}
			cp->com_text=(char*)realloc( cp->com_text,ext_len+3);//????????? ??? ??????//command + 2 chars + 0 
			//in ext_len position of zero in com_text
			switch(cp->sf.tttn){
			case 0://overflow
				cp->com_text[ext_len]='o';
				cp->com_text[ext_len+1]=0;
				break;
				
			case 1://no overflow
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='o';
				cp->com_text[ext_len+2]=0;
				break;
				
			case 2://below
				cp->com_text[ext_len]='b';
				cp->com_text[ext_len+1]=0;
				break;

			case 3://not below
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='b';
				cp->com_text[ext_len+2]=0;
				break;

			case 4://zero
				cp->com_text[ext_len]='z';
				cp->com_text[ext_len+1]=0;
				break;						
				
			case 5://not zero
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='z';
				cp->com_text[ext_len+2]=0;
				break;
				
			case 6://not above
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='a';
				cp->com_text[ext_len+2]=0;
				break;
				
			case 7://above
				cp->com_text[ext_len]='a';
				cp->com_text[ext_len+1]=0;
				break;
				
			case 8://sign
				cp->com_text[ext_len]='s';
				cp->com_text[ext_len+1]=0;
				break;
				
			case 9://no sign
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='s';
				cp->com_text[ext_len+2]=0;
				break;
				
			case 10://parity
				cp->com_text[ext_len]='p';
				cp->com_text[ext_len+1]=0;
				break;

			case 11://not parity
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='p';
				cp->com_text[ext_len+2]=0;
				break;

			case 12://less
				cp->com_text[ext_len]='l';
				cp->com_text[ext_len+1]=0;
				break;						
				
			case 13://not less
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='l';
				cp->com_text[ext_len+2]=0;
				break;
				
			case 14://not greater
				cp->com_text[ext_len]='n';
				cp->com_text[ext_len+1]='g';
				cp->com_text[ext_len+2]=0;
				break;
				
			case 15://greater
				cp->com_text[ext_len]='g';
				cp->com_text[ext_len+1]=0;
				break;
			}
			break;
			
			//f: pushf -/w/d, pusha -/w/d, popf -/w/d, popa -/w/d: modification---------------------------------------------------------------------------------------
		case 'f'://pushf -/w/d, pusha -/w/d, popf -/w/d, popa -/w/d: modification
			cp->par_count--;//modification, not parameter
			ext_len=0;
			while(cp->com_text[ext_len]!=0){
				ext_len++;
			}
			cp->com_text=(char*)realloc( cp->com_text,ext_len+2);//????????? ??? ??????//command + 1 char + 0 
			//in ext_len position of zero in com_text
			if(cp->prefix.opr==0x66){//0x66
				cp->com_text[ext_len]='w';
			}else{//no 0x66
				cp->com_text[ext_len]='d';
			}
			cp->com_text[ext_len+1]=0;
			break;
			
			//j:jcxz/jecxz: modification---------------------------------------------------------------------------------------
		case 'j'://jcxz/jecxz: modification
			cp->par_count--;//modification, not parameter
			ext_len=1;
			cp->com_text=(char*)realloc( cp->com_text,6);//????????? ??? ??????//'jecxz' + 0 

			if(cp->prefix.sr!=0x67){//0x66
				cp->com_text[1]='e';
				ext_len++;
			}
			cp->com_text[ext_len]='c';
			cp->com_text[ext_len+1]='x';
			cp->com_text[ext_len+2]='z';
			cp->com_text[ext_len+3]=0;
			break;
			
			//':'-DWORD PTR ds:[esi]-DWORD PTR es:[edi]------------------------------------------------------------------------------------
		case ':'://DWORD PTR ds:[esi]-DWORD PTR es:[edi]
			if(cp->sf.w==0){//1 byte operand 
				cp->parameters[counter][0]='B';
				cp->parameters[counter][1]='Y';
				cp->parameters[counter][2]='T';
				cp->parameters[counter][3]='E';
				ext_len=4;
			}else{//2|4 byte operand 
				if(cp->prefix.opr==0x66){//0x66 -2byte
					cp->parameters[counter][0]='W';
					cp->parameters[counter][1]='O';
					cp->parameters[counter][2]='R';
					cp->parameters[counter][3]='D';
					ext_len=4;
				}else{//no 0x66 -4byte
					cp->parameters[counter][0]='D';
					cp->parameters[counter][1]='W';
					cp->parameters[counter][2]='O';
					cp->parameters[counter][3]='R';
					cp->parameters[counter][4]='D';
					ext_len=5;
				}
			}

			cp->parameters[counter][ext_len]='_';
			cp->parameters[counter][ext_len+1]='P';
			cp->parameters[counter][ext_len+2]='T';
			cp->parameters[counter][ext_len+3]='R';
			cp->parameters[counter][ext_len+4]='_';
			ext_len+=5;

			if(counter==0){//1st par
				
				switch(cp->prefix.segr){
				case 0x2e: //cs
					cp->parameters[counter][ext_len]='C';
					break;
				case 0x36://ss
					cp->parameters[counter][ext_len]='S';
					
					break;
				case 0x26: //es
					cp->parameters[counter][ext_len]='E';
					
					break;
				case 0x64: //fs
					cp->parameters[counter][ext_len]='C';
					
					break;
				case 0x65: //gs
					cp->parameters[counter][ext_len]='G';
					break;
				default:
					cp->parameters[counter][ext_len]='D';
					break;
				}
			}else{
				cp->parameters[counter][ext_len]='E';
			}
			cp->parameters[counter][ext_len+1]='S';
			cp->parameters[counter][ext_len+2]=':';
			cp->parameters[counter][ext_len+3]='[';
			ext_len+=4;
			if(cp->prefix.sr!=0x67){//no 0x67-4byte addr
				cp->parameters[counter][ext_len]='E';
				ext_len++;
			}
			if(counter==0){//1st par
				cp->parameters[counter][ext_len]='S';
			}else{
				cp->parameters[counter][ext_len]='D';
			}
			cp->parameters[counter][ext_len+1]='I';
			cp->parameters[counter][ext_len+2]=']';
			cp->parameters[counter][ext_len+3]=0;

			break;
			
			//'g'-JMP FAR, CALL FAR------------------------------------------------------------------------------------
		case 'g'://JMP FAR, CALL FAR
			buf=255;
			ext_len=0;
			cp->parameters[counter][0]='F';
			cp->parameters[counter][1]='A';
			cp->parameters[counter][2]='R';
			cp->parameters[counter][3]='_';
			cp->parameters[counter]+=4;
			if(cp->prefix.opr==0x66){//0x67 present-4433:2211
				buf2=4;
			}else{//6655:44332211
				buf2=6;
			}
			while(ext_len!=(int)buf2){
				cp->parameters[counter][buf-255+(buf2-ext_len)*2-1]=mp[bytecounter]/16+'0';
				cp->parameters[counter][buf-255+(buf2-ext_len)*2]=mp[bytecounter]%16+'0';
				if((cp->parameters[counter][buf-255+(buf2-ext_len)*2-1]-'0')>9){				
					cp->parameters[counter][buf-255+(buf2-ext_len)*2-1]='A'+cp->parameters[counter][buf-255+(buf2-ext_len)*2-1]-'0'-10;
				}
				if((cp->parameters[counter][buf-255+(buf2-ext_len)*2]-'0')>9){
					cp->parameters[counter][buf-255+(buf2-ext_len)*2]='A'+cp->parameters[counter][buf-255+(buf2-ext_len)*2]-'0'-10;
				}
				bytecounter++;
				ext_len++;
				if((buf2-ext_len)==2){
					cp->parameters[counter][4]=':';
					buf=254;
				}
			}
			cp->parameters[counter][buf2*2+1]=0;
			cp->parameters[counter]-=4;
			break;
			
			//'x'-xlat: byte ptr ds[ebx/bx+al]------------------------------------------------------------------------------------
		case 'x'://xlat
			ext_len=0;
			cp->parameters[counter][0]='B';
			cp->parameters[counter][1]='Y';
			cp->parameters[counter][2]='T';
			cp->parameters[counter][3]='E';
			cp->parameters[counter][4]='_';
			cp->parameters[counter][5]='P';
			cp->parameters[counter][6]='T';
			cp->parameters[counter][7]='R';
			cp->parameters[counter][8]='_';

			cp->parameters[counter][9]='D';

			switch(cp->prefix.segr){
			case 0x2e:
				cp->parameters[counter][9]='C';
				break;
			case 0x36:
				cp->parameters[counter][9]='S';
				break;
			case 0x26:
				cp->parameters[counter][9]='E';
				break;
			case 0x64:
				cp->parameters[counter][9]='F';
				break;
			case 0x65:
				cp->parameters[counter][9]='G';
				break;
			}

			cp->parameters[counter][10]='S';
			cp->parameters[counter][11]=':';
			cp->parameters[counter][12]='[';

			if(cp->prefix.sr!=0x67){//ebx
				cp->parameters[counter][13]='E';
				ext_len=1;
			}

			cp->parameters[counter][13+ext_len]='B';
			cp->parameters[counter][14+ext_len]='X';
			cp->parameters[counter][15+ext_len]='+';
			cp->parameters[counter][16+ext_len]='A';
			cp->parameters[counter][17+ext_len]='L';
			cp->parameters[counter][18+ext_len]=']';
			cp->parameters[counter][19+ext_len]=0;
			break;
			
			//'q':rep/repnz влияют на описание команды: modification-------------------------------------------------------------------------------------
		case 'q'://q
			cp->par_count--;
			ext_len=strlen(cp->com_text);
			if(cp->prefix.lnr==0xf2){//repnz
				temp_char1=(char*)malloc(ext_len+7);//'repne ',0
				temp_char1[0]='r';
				temp_char1[1]='e';
				temp_char1[2]='p';
				temp_char1[3]='n';
				temp_char1[4]='z';
				temp_char1[5]='_';
				temp_char1[ext_len+7]=0;
				ext_len=6;
				
			}
			if(cp->prefix.lnr==0xf3){//rep
				temp_char1=(char*)malloc(ext_len+5);//'rep ',0
				temp_char1[0]='r';
				temp_char1[1]='e';
				temp_char1[2]='p';
				temp_char1[3]='_';
				temp_char1[ext_len+5]=0;
				ext_len=4;
			}

			if(cp->prefix.lnr==0xff){//no rep/repne
				break;
			}

			strcpy(temp_char1+ext_len,cp->com_text);//now command description modified
			free(cp->com_text);
			cp->com_text=temp_char1;
			break;
			
			//'u'-es:[edi](ins/ous)------------------------------------------------------------------------------------
		case 'u':
			if(cp->sf.w==0){//1 byte operand 
				cp->parameters[counter][0]='B';
				cp->parameters[counter][1]='Y';
				cp->parameters[counter][2]='T';
				cp->parameters[counter][3]='E';
				ext_len=4;
			}else{//2|4 byte operand 
				if(cp->prefix.opr==0x66){//0x66 -2byte
					cp->parameters[counter][0]='W';
					cp->parameters[counter][1]='O';
					cp->parameters[counter][2]='R';
					cp->parameters[counter][3]='D';
					ext_len=4;
				}else{//no 0x66 -4byte
					cp->parameters[counter][0]='D';
					cp->parameters[counter][1]='W';
					cp->parameters[counter][2]='O';
					cp->parameters[counter][3]='R';
					cp->parameters[counter][4]='D';
					ext_len=5;
				}
			}

			cp->parameters[counter][ext_len]='_';
			cp->parameters[counter][ext_len+1]='P';
			cp->parameters[counter][ext_len+2]='T';
			cp->parameters[counter][ext_len+3]='R';
			cp->parameters[counter][ext_len+4]='_';
			cp->parameters[counter][ext_len+5]='E';
			cp->parameters[counter][ext_len+6]='S';
			cp->parameters[counter][ext_len+7]=':';
			cp->parameters[counter][ext_len+8]='[';
			ext_len+=9;
			if(cp->prefix.sr!=0x67){//edi
				cp->parameters[counter][ext_len]='E';
				ext_len++;
			}
			
			cp->parameters[counter][ext_len]='D';
			
			cp->parameters[counter][ext_len+1]='I';
			cp->parameters[counter][ext_len+2]=']';
			cp->parameters[counter][ext_len+3]=0;

			break;
			
			//-----------------------------------------------imm as memory (mov eax,full displacement)--------------------------------------------------	
		case 'h'://imm as memory (mov eax,full displacement)//0x66/0x67/segment redefinition/w/d affects

			if(cp->sf.w==0){//1 byte operand 
				cp->parameters[counter][0]='B';
				cp->parameters[counter][1]='Y';
				cp->parameters[counter][2]='T';
				cp->parameters[counter][3]='E';
				ext_len=4;
			}else{//2|4 byte operand 
				if(cp->prefix.opr==0x66){//0x66 -2byte
					cp->parameters[counter][0]='W';
					cp->parameters[counter][1]='O';
					cp->parameters[counter][2]='R';
					cp->parameters[counter][3]='D';
					ext_len=4;
				}else{//no 0x66 -4byte
					cp->parameters[counter][0]='D';
					cp->parameters[counter][1]='W';
					cp->parameters[counter][2]='O';
					cp->parameters[counter][3]='R';
					cp->parameters[counter][4]='D';
					ext_len=5;
				}
			}

			cp->parameters[counter][ext_len]='_';
			cp->parameters[counter][ext_len+1]='P';
			cp->parameters[counter][ext_len+2]='T';
			cp->parameters[counter][ext_len+3]='R';
			cp->parameters[counter][ext_len+4]='_';
			ext_len+=5;

			switch(cp->prefix.segr){
			case 0x2e: //cs
				cp->parameters[counter][ext_len]='C';
				break;
			case 0x36://ss
				cp->parameters[counter][ext_len]='S';
				break;
			case 0x26: //es
				cp->parameters[counter][ext_len]='E';
				break;
			case 0x64: //fs
				cp->parameters[counter][ext_len]='C';
				break;
			case 0x65: //gs
				cp->parameters[counter][ext_len]='G';
				break;
			default:
				cp->parameters[counter][ext_len]='D';
				break;
			}

			cp->parameters[counter][ext_len+1]='S';
			cp->parameters[counter][ext_len+2]=':';
			cp->parameters[counter][ext_len+3]='[';
			ext_len+=4;

			if(cp->prefix.sr==0x67){//2 bytes imm
				buf =2;
			}else{//4 bytes imm
				buf=4;
			}

			cp->parameters[counter][ext_len+buf*2+1]=0;
			cp->parameters[counter][ext_len+buf*2]=']';

			while(buf!=0){
				cp->parameters[counter][ext_len+buf*2-1]=mp[bytecounter]%16+'0';
				//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
				//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
				if((cp->parameters[counter][ext_len+buf*2-1]-'0')>9){				
					cp->parameters[counter][ext_len+buf*2-1]='A'+cp->parameters[counter][ext_len+buf*2-1]-'0'-10;
				}
				cp->parameters[counter][ext_len+buf*2-2]=mp[bytecounter]/16+'0';
				//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
				if((cp->parameters[counter][ext_len+buf*2-2]-'0')>9){
					cp->parameters[counter][ext_len+buf*2-2]='A'+cp->parameters[counter][ext_len+buf*2-2]-'0'-10;
				}
				bytecounter++;
				buf--;
			}

			break;
			
			//-------------------------------------------------Call/jmp far intersegment — memory operand redefinition---------------------------------------------------				
case 'k'://Call/jmp far intersegment
cp->par_count--;
buf=0;//if 0x66 is present
if(cp->prefix.opr==0x66){
	buf=1;
}
ext_len=5+buf+strlen(cp->parameters[0]);//'FAR '+ strlen(cp->parameters[0]),0
temp_char1=(char*)malloc(ext_len);
temp_char1[ext_len-1]=0;
temp_char1[0]='F';
temp_char1[1]='A';
temp_char1[2]='R';
temp_char1[3]='_';
if(cp->prefix.opr==0x66){
	temp_char1[4]='D';//dword
}else{
	temp_char1[4]='F';//fword
}
strcpy(temp_char1+5,cp->parameters[0]+1-buf);
//printf("\nTEMP_CHAR1=%s\n",temp_char1);
free(cp->parameters[0]);
cp->parameters[0]=temp_char1;
break;
			
		}
		counter++;
	}
	//-----------------------------------------------operands parsing----------------------------------------------------------------------------------------------------------------------------------------------------------	
	
	fseek(hFile,ftell(hFile)-com_len1+bytecounter,SEEK_SET);
	fclose(hFileOpcodes);
	return RET_SUCCESS;
}

//====================================================================================================
char*CharEnter(){
	char*res=NULL;
	int i=0;
	do{
		i++;
		res=(char*)realloc(res,i);
		res[i-1]=(char)getchar();
	}while(res[i-1]!='\n');
	res[i-1]=0;
	return res;
}

//====================================================================================================
IMAGE_SECTION_HEADER findCodeSection(FILE* hFile) {
	//TODO findCodeSection should return code section file offset and file size, not IMAGE_SECTION_HEADER
	//Search code section by AddressOfEntryPoint value
	IMAGE_SECTION_HEADER res = { 0 };
	DWORD int_buf;
	WORD word_buf;
	WORD NumberOfSections;
	DWORD AddressOfEntryPoint;
	int i = 0;
	fseek(hFile, 0x3c, SEEK_SET);
	fread(&int_buf, 4, 1, hFile); //'PE' offset in int_buf
	//printf("e_lfanew=%#x\n",int_buf);
	fseek(hFile, int_buf + 6, SEEK_SET);//fp on NumberOfSections
	fread(&NumberOfSections, 2, 1, hFile);
	//int_buf=int_buf+sizeof(IMAGE_FILE_HEADER);//fp on SizeOfOptionalHeader-int_buf+sizeof(IMAGE_FILE_HEADER)+4/*'PE'*/-4/*2 positions back in IMAGE_FILE_HEADER*/
	fseek(hFile, int_buf + sizeof(IMAGE_FILE_HEADER), SEEK_SET);
	fread(&word_buf, 2, 1, hFile);//SizeOfOptionalHeader in word_buf
	printf("SizeOfOptionalHeader=0x%04x\n", word_buf);
	fseek(hFile, 18, SEEK_CUR);//fp on AddressOfEntryPoint
	fread(&AddressOfEntryPoint, 4, 1, hFile);
	printf("AddressOfEntryPoint=0x%08x\n", AddressOfEntryPoint);
	printf("NumberOfSections=0x%04x\n", NumberOfSections);
	fseek(hFile, word_buf - 20, SEEK_CUR);//fp on 1st section
	while (i != NumberOfSections) {
		fread(&res, sizeof(IMAGE_SECTION_HEADER), 1, hFile);
		if ((AddressOfEntryPoint >= res.VirtualAddress) && (AddressOfEntryPoint < (res.VirtualAddress + res.Misc.VirtualSize))) {
			return res;
		}
		i++;
	}
	printf("[ERROR] Failed to find code section.\n");
	return res;
}

//====================================================================================================
int main(){
	FILE*hFile;
	SCommand command;
	int i=1;
	char*path;
	IMAGE_SECTION_HEADER sSection;
	
	printf("Enter path: ");//opening file
	path=CharEnter();
	printf("\tTarget PE: %s\n",path);
	
	if((hFile=fopen(path,"rb"))==0){
		printf("Error: opening file failed.\n");
		free(path);
		getch();
		return 1;
	}
	
	//Multiple code sections can exist
	sSection=findCodeSection(hFile);
	
	fseek(hFile,sSection.PointerToRawData,SEEK_SET);
	
	while((GetCommand(&command,hFile)==RET_SUCCESS)&&(ftell(hFile)<(sSection.PointerToRawData+sSection.SizeOfRawData))){
		printf("%s",command.com_text);
		if(command.par_count!=0){
			printf(" %s",command.parameters[0]);
		}
		while(i<command.par_count){
			printf(", %s",command.parameters[i]);
			i++;
		}
		printf("\n");
	}
	
	fclose(hFile);
	free(path);
	getch();
	
	return 0;
}
