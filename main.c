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

// Platform abstraction defines
#define ERR_INVALID_ARGS	ERROR_BAD_ARGUMENTS
#define ERR_FILE_OPERATION	ERROR_FILE_INVALID
#define RET_SUCCESS 0
#define RET_ERROR 1
#define RET_FILE_END 2

#define LINE_LEN 62
#define OPCODE_FIELD_OFFSET 0
#define EXT_FIELD_OFFSET 33
#define MODRM_FIELD_OFFSET 25
#define DESCRIPTION_FIELD_OFFSET 40

#define FILEPATH_OPCODES "opcodes.txt" //TODO TEXT("opcodes.txt")

//====================================================================================================
void SCommand_Init(SCommand* o_pObjCommand)
{
	int idxOpcodeByte = 0;

	o_pObjCommand->m_objPrefix.m_nPrefixCount = 0;
	o_pObjCommand->m_objPrefix.m_pBytes = NULL;
	o_pObjCommand->m_objPrefix.m_nLnr = 255; //TODO values in hex representation
	o_pObjCommand->m_objPrefix.m_nSegr = 255;
	o_pObjCommand->m_objPrefix.m_nOpr = 255;
	o_pObjCommand->m_objPrefix.m_nSr = 255;

	o_pObjCommand->m_objOpcode.m_nByteNum = 0;
	o_pObjCommand->m_objOpcode.m_strBits = (char*)malloc(25); //24 bits + 0
	for (; idxOpcodeByte < 25; ++idxOpcodeByte) {
		o_pObjCommand->m_objOpcode.m_strBits[idxOpcodeByte] = 0;
	}

	o_pObjCommand->m_objOpcode.m_pHexBytes = (unsigned char*)malloc(3);

	o_pObjCommand->m_objSpecialFields.m_nW = -128;
	o_pObjCommand->m_objSpecialFields.m_nS = -128;
	o_pObjCommand->m_objSpecialFields.m_nD = -128;
	o_pObjCommand->m_objSpecialFields.m_nUuu = -128;
	o_pObjCommand->m_objSpecialFields.m_nFf = -128;
	o_pObjCommand->m_objSpecialFields.m_nReg = -128;
	o_pObjCommand->m_objSpecialFields.m_nEee = -128;
	o_pObjCommand->m_objSpecialFields.m_nTttn = -128;

	o_pObjCommand->m_objModrm.m_nMod = -128;
	o_pObjCommand->m_objModrm.m_nReg = -128;
	o_pObjCommand->m_objModrm.m_nRm = -128;

	o_pObjCommand->m_objSib.m_nScale = -128;
	o_pObjCommand->m_objSib.m_nIndex = -128;
	o_pObjCommand->m_objSib.m_nBase = -128;
	o_pObjCommand->m_objSib.m_nSib = 255;

	o_pObjCommand->m_strCommandText = NULL;
	o_pObjCommand->m_pParameters = NULL;
	o_pObjCommand->m_strParameters = NULL;
	o_pObjCommand->m_nParameterCount = 0;
	return;
}

//====================================================================================================
void SCommand_Destroy(SCommand* o_pObjCommand)
{
	int idxParam = 0;
	size_t nParameterCount = o_pObjCommand->m_strParameters == NULL ? 0 : strlen(o_pObjCommand->m_strParameters);
	if (o_pObjCommand->m_pParameters != NULL) {
		while (idxParam < nParameterCount) {
			free(o_pObjCommand->m_pParameters[idxParam]);
			++idxParam;
		}
		free(o_pObjCommand->m_pParameters);
	}
	free(o_pObjCommand->m_objPrefix.m_pBytes);
	free(o_pObjCommand->m_objOpcode.m_strBits);
	free(o_pObjCommand->m_objOpcode.m_pHexBytes);
	free(o_pObjCommand->m_strCommandText);
	free(o_pObjCommand->m_strParameters);
	memset(o_pObjCommand, 0, sizeof(*o_pObjCommand));
}

//====================================================================================================
char* convertByteToBitString(UByte* i_pByte)
{
	char* pBitsStrRepr;
	pBitsStrRepr = (char*)malloc(9);//mem allocating
	pBitsStrRepr[0]=(char)i_pByte->m_objUByte.m_nBit7+'0';
	pBitsStrRepr[1]=(char)i_pByte->m_objUByte.m_nBit6+'0';
	pBitsStrRepr[2]=(char)i_pByte->m_objUByte.m_nBit5+'0';
	pBitsStrRepr[3]=(char)i_pByte->m_objUByte.m_nBit4+'0';
	pBitsStrRepr[4]=(char)i_pByte->m_objUByte.m_nBit3+'0';
	pBitsStrRepr[5]=(char)i_pByte->m_objUByte.m_nBit2+'0';
	pBitsStrRepr[6]=(char)i_pByte->m_objUByte.m_nBit1+'0';
	pBitsStrRepr[7]=(char)i_pByte->m_objUByte.m_nBit0+'0';
	pBitsStrRepr[8]=0;
	return pBitsStrRepr;
}

//====================================================================================================
int findCommand(FILE* i_hOpcodeFile,const char* i_strOpcodeBits, char* i_strModrmBits)
{
	int idxTextOffset=0,idxOpcode=0;
	char nBufferByte,nBufferByte2;
	while (1) {
		fseek(i_hOpcodeFile,idxTextOffset,SEEK_SET);
		if (feof(i_hOpcodeFile)){
			return -1;
		}
		while (1){
			fread(&nBufferByte,1,1,i_hOpcodeFile);
			//printf("\tbuf:%c text_counter:%d\n",buf,text_counter);
			//getch();
			if (i_strOpcodeBits[idxOpcode]==0){
				if (nBufferByte!=' ' && nBufferByte!='|'){
					//printf("\tNOT FOUND 1\n");
					idxOpcode=0;
					break;
				}
				
				
				//now modrm testing
				//printf("\t\tStarting to tezt reg field:\n");
				fseek(i_hOpcodeFile,idxTextOffset+MODRM_FIELD_OFFSET,SEEK_SET);//check is modrm present
				fread(&nBufferByte,1,1,i_hOpcodeFile);
				if (nBufferByte==' '){//modrm isnt here
					return idxTextOffset;
				}
				
				if (nBufferByte=='m'){//md
					if ((i_strModrmBits[0]=='1')&&(i_strModrmBits[1]=='1')){
						idxOpcode=0;
						break;
					}
				}else{//11
					if (nBufferByte==i_strModrmBits[0]){
						fread(&nBufferByte,1,1,i_hOpcodeFile);
						if (nBufferByte!=i_strModrmBits[1]){
							idxOpcode=0;
							break;
						}
					}else{
						idxOpcode=0;
						break;
					}
				}				
				
				fseek(i_hOpcodeFile,idxTextOffset+MODRM_FIELD_OFFSET+3,SEEK_SET);//+3 for reg/opcode field
				fread(&nBufferByte,1,1,i_hOpcodeFile);
				
				if (nBufferByte!='0' && nBufferByte!='1'){
					//printf("FindCommand:2\n");
					return idxTextOffset;
				}
				
				if (i_strModrmBits[2]!=nBufferByte){
					idxOpcode=0;
					//printf("\tNOT FOUND -2\n");
					break;
				}
				fread(&nBufferByte,1,1,i_hOpcodeFile);
				if (i_strModrmBits[3]!=nBufferByte){
					idxOpcode=0;
					//printf("\tNOT FOUND 3\n");
					break;
				}
				fread(&nBufferByte,1,1,i_hOpcodeFile);
				if (i_strModrmBits[4]!=nBufferByte){
					idxOpcode=0;
					//printf("\tNOT FOUND 4\n");
					break;
				}
				//printf("FindCommand:3\n");
				return idxTextOffset;
			}
			
			if ((nBufferByte!=i_strOpcodeBits[idxOpcode]) && (nBufferByte=='0' || nBufferByte=='1' || nBufferByte==' ')){
				//printf("\tNOT FOUND 5 opcode_counter:%d text_counter=%d\n\t\tbuf=%c p=%c\n",opcode_counter,text_counter,buf,p[opcode_counter]);
				idxOpcode=0;
				break;
			}
			
			if (nBufferByte=='f'){//pop s - push s processing
				fseek(i_hOpcodeFile,ftell(i_hOpcodeFile)+3,SEEK_SET);//0-push,1-pop
				fread(&nBufferByte,1,1,i_hOpcodeFile);
				if (nBufferByte=='1'){//pop
					if (((i_strOpcodeBits[idxOpcode]-'0')==0)&&((i_strOpcodeBits[idxOpcode+1]-'0')==1)){//01-cs
						idxOpcode=0;
						break;
					}
				}
				fseek(i_hOpcodeFile,ftell(i_hOpcodeFile)-3,SEEK_SET);
				++idxOpcode;
			}
			
			if (nBufferByte=='u'){//uuu-sreg3 //pop s
				nBufferByte2=(i_strOpcodeBits[idxOpcode]-'0')*4+(i_strOpcodeBits[idxOpcode+1]-'0')*2+i_strOpcodeBits[idxOpcode+2]-'0';
				if ((nBufferByte2!=4)&&(nBufferByte2!=5)){//buf2!=100(fs) & buf2!=101(gs)
					idxOpcode=0;
					break;
				}
				fread(&nBufferByte,1,1,i_hOpcodeFile);
				fread(&nBufferByte,1,1,i_hOpcodeFile);
				idxOpcode+=2;
			}
			
			++idxOpcode;
		}
		idxTextOffset+=LINE_LEN;
		while (1){//comments
			fseek(i_hOpcodeFile,idxTextOffset,SEEK_SET);
			if (fread(&nBufferByte,1,1,i_hOpcodeFile)==0){//file end
				return -1;
			}
			
			//printf("\t2) buf:%c text_counter:%d\n",buf,text_counter);
			if (nBufferByte!=';'){
				// printf("\tbrokenhearted\n");
				break;	
			}
			idxTextOffset+=LINE_LEN;
		}
	}
}

//====================================================================================================
int SCommand_ParsePrefixes(SCommand* o_pCommand,const unsigned char* i_pBytes,int i_nByteCount)
{
	char nBufferByte=0;
	int idxByte=0;
	while (idxByte < i_nByteCount) {
		//buf is used as flag that we can exit from while
		switch(i_pBytes[idxByte]){
			//1 group
		case 0x66://redefinition of operand size
			o_pCommand->m_objPrefix.m_nOpr=0x66;//setting flag
			break;	
			
			//2 group			
		case 0x67://redefinition of address size
			o_pCommand->m_objPrefix.m_nSr=0x67;//setting flag
			break;
			
			//3 group			
		case 0xF0://LOCK
		case 0xF2://REPNZ
		case 0xF3://REP
			o_pCommand->m_objPrefix.m_nLnr=i_pBytes[idxByte];//setting flag
			break;
			
			//4 group			
		case 0x2E: //CS
		case 0x36://SS
		case 0x3E: //DS
		case 0x26://ES
		case 0x64://FS
		case 0x65://GS
			o_pCommand->m_objPrefix.m_nSegr=i_pBytes[idxByte];//setting flag
			break;
			
		default:
			nBufferByte=1;
		}
		if (nBufferByte==1){//exit
			break;
		}
		++o_pCommand->m_objPrefix.m_nPrefixCount;
		o_pCommand->m_objPrefix.m_pBytes=(unsigned char*)realloc(o_pCommand->m_objPrefix.m_pBytes,o_pCommand->m_objPrefix.m_nPrefixCount);
		o_pCommand->m_objPrefix.m_pBytes[o_pCommand->m_objPrefix.m_nPrefixCount-1]=i_pBytes[idxByte];
		++idxByte;
	}
	if (idxByte==i_nByteCount){
		return RET_ERROR;
	}
	return RET_SUCCESS;
}

//====================================================================================================
void SCommand_Print(const SCommand* i_pCommand)
{
	int idx=0;
	printf("SCommand:\n");
	
	printf("PREFIX:\n");//prefix
	printf("\tprefixcount:%d\n",i_pCommand->m_objPrefix.m_nPrefixCount);
	printf("\tp:");
	while (idx<i_pCommand->m_objPrefix.m_nPrefixCount){
		printf(" %x",		i_pCommand->m_objPrefix.m_pBytes[idx]);
		++idx;
	}
	printf("\n");
	printf("\tlnr:%x\n",i_pCommand->m_objPrefix.m_nLnr);
	printf("\tsegr:%x\n",i_pCommand->m_objPrefix.m_nSegr);
	printf("\topr:%x\n",i_pCommand->m_objPrefix.m_nOpr);
	printf("\tsr:%x\n",i_pCommand->m_objPrefix.m_nSr);
	
	printf("OPCODE:\n");//opcodes
	printf("\tbytenum:%d\n",i_pCommand->m_objOpcode.m_nByteNum);
	printf("\thex_bytes:");
	idx=0;
	while (idx<i_pCommand->m_objOpcode.m_nByteNum){
		printf(" %#X",i_pCommand->m_objOpcode.m_pHexBytes[idx]);
		++idx;
	}
	printf("\n");
	printf("\tbytes:%s\n",i_pCommand->m_objOpcode.m_strBits);
	
	printf("EXT:\n");
	printf("\tw=%d s=%d d=%d reg=%d eee=%d uuu=%d ff=%d tttn=%d\n",i_pCommand->m_objSpecialFields.m_nW,i_pCommand->m_objSpecialFields.m_nS,i_pCommand->m_objSpecialFields.m_nD,i_pCommand->m_objSpecialFields.m_nReg,i_pCommand->m_objSpecialFields.m_nEee,i_pCommand->m_objSpecialFields.m_nUuu,i_pCommand->m_objSpecialFields.m_nFf,i_pCommand->m_objSpecialFields.m_nTttn);
	
	printf("MODRM:\n");
	printf("\tmodrm:%#X mod:%d reg:%d rm:%d\n",i_pCommand->m_objModrm.m_nModrm,i_pCommand->m_objModrm.m_nMod,i_pCommand->m_objModrm.m_nReg,i_pCommand->m_objModrm.m_nRm);
	printf("Command:\n");
	printf("\tcom_text:%s\n",i_pCommand->m_strCommandText);
	printf("\tpar:%s parcount:%d\n",i_pCommand->m_strParameters,i_pCommand->m_nParameterCount);
	idx=0;
	printf("\tparameters:\n");
	while (idx<i_pCommand->m_nParameterCount){
		printf("\t\t%d: %s\n",idx,i_pCommand->m_pParameters[idx]);
		++idx;
	}
	printf("\nRESULT: %s",i_pCommand->m_strCommandText);//printing result
	if (i_pCommand->m_nParameterCount!=0){
		printf(" %s",i_pCommand->m_pParameters[0]);
		idx=1;
		while (idx<i_pCommand->m_nParameterCount){
			printf(", %s",i_pCommand->m_pParameters[idx]);
			++idx;
		}
	}
	printf("\n");
	
}

//====================================================================================================
int SCommand_GetCommand(SCommand* o_pCommand, FILE* i_hTargetFile, int i_nAvailableBytes)
{
	UByte objTempByte;
	FILE*hOpcodeFile;
	int idxByte=0,nCommandOffset,nExtensionLength=0,idxOperand=0,nCommandLength,nBytesRead;
	unsigned char* pBytes;
	char* strBitString;
	char* strTempText;
	unsigned char nBufferByte,nBufferByte2;
	
	//printf("Begin GetCommand:\n");


	if ((hOpcodeFile=fopen(FILEPATH_OPCODES,"rb"))==0){
		printf("\tError: cant open \"%s\"\n", FILEPATH_OPCODES);
		return RET_ERROR;
	}
	pBytes=(unsigned char*)malloc(MAX_COMMAND_LEN);//mem allocating
	nBytesRead=fread(pBytes, 1, i_nAvailableBytes, i_hTargetFile);//reading data block, !file pointer+=MAX_COMMAND_LEN!
	
	if (nBytesRead==0){//file end
		return RET_FILE_END;
	}
	
	//printf("\"prefixes.ttt\" opened\n");
	/*------------------------------------------------------------prefix parsing------------------------------------------------------*/
	if (SCommand_ParsePrefixes(o_pCommand,pBytes,nBytesRead)==RET_ERROR){
		printf("\tError: invalid command\n");
		return RET_ERROR;
	}
	idxByte+=o_pCommand->m_objPrefix.m_nPrefixCount;
	/*------------------------------------------------------------prefix parsing------------------------------------------------------*/	
	
	/*------------------------------------------------------------command finding-----------------------------------------------------*/	
#ifdef _DEBUG
	printf("\tCommand finding:\n");
#endif
	
	nBufferByte=0;//like current command byte counter
	while (nBufferByte < 3){//coz buf=0
		objTempByte.m_nByte=pBytes[idxByte];
		strBitString=convertByteToBitString(&objTempByte);
		strcpy(o_pCommand->m_objOpcode.m_strBits+nBufferByte*8,strBitString);
		o_pCommand->m_objOpcode.m_pHexBytes[nBufferByte]=pBytes[idxByte];
		++o_pCommand->m_objOpcode.m_nByteNum;
		free(strBitString);
		++idxByte;
		objTempByte.m_nByte=pBytes[idxByte];//possible modrm
		strBitString=convertByteToBitString(&objTempByte);
#ifdef _DEBUG
		printf("\t\tPossible opcode:%s possible modrm:%s\n",o_pCommand->m_objOpcode.m_strBits,strBitString);
#endif
		if ((nCommandOffset=findCommand(hOpcodeFile,o_pCommand->m_objOpcode.m_strBits,strBitString))!=-1){//command found
			break;//we can exit, buf<4
		}
		++nBufferByte;
	}
	
	if (nBufferByte==3){//command not found
		printf("\tError:command not found!\n");
		fclose(hOpcodeFile);
		return RET_ERROR;
	}
	//now in temp_int command position 
#ifdef _DEBUG
	printf("\tCommand position:%d\n",nCommandOffset);
#endif
	/*------------------------------------------------------------command finding-----------------------------------------------------*/	
	
	/*------------------------------------------------------------ext parsing---------------------------------------------------------*/	

	fseek(hOpcodeFile,nCommandOffset,SEEK_SET);//fp on command field
	nExtensionLength=0;
	while (1){
		fread(&nBufferByte,1,1,hOpcodeFile);
		if (nBufferByte==' '){//cell end
			break;
		}
		switch(nBufferByte){
		case 's'://s
			o_pCommand->m_objSpecialFields.m_nS=o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0';
			break;
			
		case 'w'://w
			o_pCommand->m_objSpecialFields.m_nW=o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0';
			break;
			
		case 'd'://d
			o_pCommand->m_objSpecialFields.m_nD=o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0';
			break;
			
		case 'r'://reg
			o_pCommand->m_objSpecialFields.m_nReg=(o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0')*4+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+1]-'0')*2+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+2]-'0');
			nExtensionLength+=2;
			fread(&nBufferByte,1,1,hOpcodeFile);
			fread(&nBufferByte,1,1,hOpcodeFile);
			break;
			
		case 't'://tttn
			o_pCommand->m_objSpecialFields.m_nTttn=(o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0')*8+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+1]-'0')*4+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+2]-'0')*2+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+3]-'0');
			nExtensionLength+=3;
			fread(&nBufferByte,1,1,hOpcodeFile);
			fread(&nBufferByte,1,1,hOpcodeFile);
			fread(&nBufferByte,1,1,hOpcodeFile);
			break;
		case 'e'://eee special purpose registers (cr, dr)
			o_pCommand->m_objSpecialFields.m_nEee=(o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0')*4+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+1]-'0')*2+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+2]-'0');
			nExtensionLength+=2;
			fread(&nBufferByte,1,1,hOpcodeFile);
			fread(&nBufferByte,1,1,hOpcodeFile);
			break;
		case 'f'://ff-sreg2(2 bits) 
			o_pCommand->m_objSpecialFields.m_nFf=(o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0')*2+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+1]-'0');
			nExtensionLength+=1;
			fread(&nBufferByte,1,1,hOpcodeFile);
			break;
		case 'u'://uuu-sreg3(3 bits)
			o_pCommand->m_objSpecialFields.m_nUuu=(o_pCommand->m_objOpcode.m_strBits[nExtensionLength]-'0')*4+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+1]-'0')*2+(o_pCommand->m_objOpcode.m_strBits[nExtensionLength+2]-'0');
			nExtensionLength+=2;
			fread(&nBufferByte,1,1,hOpcodeFile);
			fread(&nBufferByte,1,1,hOpcodeFile);
			break;
			/*default:
			printf("\tError:cant find ext. buf=%c\n",buf);
			getch();
			return -1;*/	
		}
		++nExtensionLength;
	}
	/*------------------------------------------------------------ext parsing---------------------------------------------------------*/	
	/*----------------------------------------------------------modrm parsing---------------------------------------------------------*/	
	fseek(hOpcodeFile,nCommandOffset+MODRM_FIELD_OFFSET,SEEK_SET);//check modrm is present
	fread(&nBufferByte,1,1,hOpcodeFile);
	if (nBufferByte!=' '){//modrm is here
		o_pCommand->m_objModrm.m_nModrm=pBytes[idxByte];
		o_pCommand->m_objModrm.m_nMod=(strBitString[0]-'0')*2+(strBitString[1]-'0');
		o_pCommand->m_objModrm.m_nReg=(strBitString[2]-'0')*4+(strBitString[3]-'0')*2+(strBitString[4]-'0');
		o_pCommand->m_objModrm.m_nRm=(strBitString[5]-'0')*4+(strBitString[6]-'0')*2+(strBitString[7]-'0');
		++idxByte;
	}
	free(strBitString);//temp_char:possible modrm
#ifdef _DEBUG
	printf("\tMODRM:mod:%d reg:%d rm:%d\n",o_pCommand->m_objModrm.m_nMod,o_pCommand->m_objModrm.m_nReg,o_pCommand->m_objModrm.m_nRm);
#endif
	/*----------------------------------------------------------modrm parsing---------------------------------------------------------*/

	/*--------------------------------------------------------command parsing---------------------------------------------------------*/
	fseek(hOpcodeFile,nCommandOffset+DESCRIPTION_FIELD_OFFSET,SEEK_SET);
	//printf("\tfseek=%d temp_int+DESCRIPTION_FIELD_OFFSET=%d\n",ftell(hFileOpcodes),temp_int+DESCRIPTION_FIELD_OFFSET);
	nExtensionLength=0;
	while (1){//readindg command
		fread(&nBufferByte,1,1,hOpcodeFile);
		if (nBufferByte==' '){
			break;
		}
		o_pCommand->m_strCommandText=(char*)realloc( o_pCommand->m_strCommandText,nExtensionLength+2);
		o_pCommand->m_strCommandText[nExtensionLength]=nBufferByte;
		++nExtensionLength;
	}
	o_pCommand->m_strCommandText[nExtensionLength]=0;
#ifdef _DEBUG
	printf("\tcom_text:%s\n",o_pCommand->m_strCommandText);
#endif
	/*--------------------------------------------------------command parsing---------------------------------------------------------*/

	/*--------------------------------------------------------operand parsing---------------------------------------------------------*/
	//operands reading------------
	nExtensionLength=0;
	while (1){
		o_pCommand->m_strParameters=(char*)realloc(o_pCommand->m_strParameters,nExtensionLength+2);
		fread(&nBufferByte,1,1,hOpcodeFile);
		if (nBufferByte==' '){// no operands | file end
			break;
		}
		
		o_pCommand->m_strParameters[nExtensionLength]=nBufferByte;
		++o_pCommand->m_nParameterCount;
		++nExtensionLength;
	}	
	o_pCommand->m_strParameters[nExtensionLength]=0;
	
	//exchanging if d==0---------------------	
	if ((o_pCommand->m_objSpecialFields.m_nD==0) && (o_pCommand->m_nParameterCount>=2)){//exchanging ('d' field) if not immediate as 1st par
		if (o_pCommand->m_strParameters[1]!='i'){
			nBufferByte=o_pCommand->m_strParameters[1];
			o_pCommand->m_strParameters[1]=o_pCommand->m_strParameters[0];
			o_pCommand->m_strParameters[0]=nBufferByte;
		}
	}
	
	o_pCommand->m_pParameters=(char**)malloc(sizeof(char*)*(o_pCommand->m_nParameterCount));//memory allocating
	nExtensionLength=0;
	while (nExtensionLength<o_pCommand->m_nParameterCount){
		o_pCommand->m_pParameters[nExtensionLength]=(char*)malloc(40);//[eax+ebx*1+0FFFFFFFF],0
		++nExtensionLength;
	}
	//printf("DEBUG");	
	//-----------------------------------------------operands parsing----------------------------------------------------------------------------------------------------------------------------------------------------------	
	while (idxOperand<o_pCommand->m_nParameterCount){
		switch(o_pCommand->m_strParameters[idxOperand]){
			
		case 'r'://reg1
			if (o_pCommand->m_objSpecialFields.m_nReg!=-128){//reg (reg field is always last 3 bytes of opcode)
				nExtensionLength=o_pCommand->m_objSpecialFields.m_nReg;
				//printf("\t\tpos in ret:%d temp_char:%s\n",Find(hFileRet,temp_char1),temp_char1);
			}else{//search in mod/opcode section
				fseek(hOpcodeFile,nCommandOffset+MODRM_FIELD_OFFSET+3,SEEK_SET);//fp on mod/opcode section
				fread(&nBufferByte,1,1,hOpcodeFile);
				if (nBufferByte=='r'){
					//check if reg1
					fseek(hOpcodeFile,nCommandOffset+MODRM_FIELD_OFFSET+6,SEEK_SET);
					fread(&nBufferByte,1,1,hOpcodeFile);
					if (nBufferByte=='1' || nBufferByte==' '){//reg1 | reg
						nExtensionLength=o_pCommand->m_objModrm.m_nReg;
					}else{//reg1 in rm section
						nExtensionLength=o_pCommand->m_objModrm.m_nRm;
					}
				}else{//reg1 in rm section
					nExtensionLength=o_pCommand->m_objModrm.m_nRm;
				}
			}
			//in ext_len number of register
			switch (nExtensionLength){
			case 0://al ax eax
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='a';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='a';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='a';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
			case 1://ecx
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='c';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='c';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='c';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
				
				
			case 2://edx
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='d';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='d';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='d';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;

			case 3://ebx
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='b';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='b';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='b';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;

			case 4://esp
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='s';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='s';
						o_pCommand->m_pParameters[idxOperand][1]='p';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='s';
						o_pCommand->m_pParameters[idxOperand][2]='p';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;						
				
			case 5://ebp
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='b';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='b';
						o_pCommand->m_pParameters[idxOperand][1]='p';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='b';
						o_pCommand->m_pParameters[idxOperand][2]='p';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
				
			case 6://esi
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='e';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='s';
						o_pCommand->m_pParameters[idxOperand][1]='i';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='s';
						o_pCommand->m_pParameters[idxOperand][2]='i';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
				
			case 7://edi
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='d';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='d';
						o_pCommand->m_pParameters[idxOperand][1]='i';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='d';
						o_pCommand->m_pParameters[idxOperand][2]='i';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
			}
			break;
			
			//------------------------------------reg2----------------------------------------------------------------------			
		case 'p'://reg2
			fseek(hOpcodeFile,nCommandOffset+MODRM_FIELD_OFFSET+3,SEEK_SET);//fp on reg/opcode section
			fread(&nBufferByte,1,1,hOpcodeFile);
			if (nBufferByte=='r'){//check if reg2
				fseek(hOpcodeFile,nCommandOffset+MODRM_FIELD_OFFSET+6,SEEK_SET);
				fread(&nBufferByte,1,1,hOpcodeFile);
				if (nBufferByte=='2'){//reg2
					nExtensionLength=o_pCommand->m_objModrm.m_nReg;
				}else{//reg2 in rm section
					nExtensionLength=o_pCommand->m_objModrm.m_nRm;
				}
			}else{//reg2 in rm section
				nExtensionLength=o_pCommand->m_objModrm.m_nRm;
			}	
			//in ext_len number of register
			switch (nExtensionLength){
			case 0://al ax eax
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='a';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='a';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='a';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
			case 1://ecx
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='c';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='c';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='c';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
				
				
			case 2://edx
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='d';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='d';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='d';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;

			case 3://ebx
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='b';
					o_pCommand->m_pParameters[idxOperand][1]='l';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='b';
						o_pCommand->m_pParameters[idxOperand][1]='x';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='b';
						o_pCommand->m_pParameters[idxOperand][2]='x';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;

			case 4://esp
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='s';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='s';
						o_pCommand->m_pParameters[idxOperand][1]='p';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='s';
						o_pCommand->m_pParameters[idxOperand][2]='p';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;						
				
			case 5://ebp
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='b';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='b';
						o_pCommand->m_pParameters[idxOperand][1]='p';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='b';
						o_pCommand->m_pParameters[idxOperand][2]='p';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
				
			case 6://esi
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='e';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='s';
						o_pCommand->m_pParameters[idxOperand][1]='i';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='s';
						o_pCommand->m_pParameters[idxOperand][2]='i';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
				
			case 7://edi
				if (o_pCommand->m_objSpecialFields.m_nW==0){
					o_pCommand->m_pParameters[idxOperand][0]='d';
					o_pCommand->m_pParameters[idxOperand][1]='d';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{
					if (o_pCommand->m_objPrefix.m_nOpr==0x66){//66h - operand-size prefix
						o_pCommand->m_pParameters[idxOperand][0]='d';
						o_pCommand->m_pParameters[idxOperand][1]='i';
						o_pCommand->m_pParameters[idxOperand][2]=0;
						break;
					}else{
						o_pCommand->m_pParameters[idxOperand][0]='e';
						o_pCommand->m_pParameters[idxOperand][1]='d';
						o_pCommand->m_pParameters[idxOperand][2]='i';
						o_pCommand->m_pParameters[idxOperand][3]=0;
					}
				}
				break;
			}
			break;
			
			//-------------------------------------------------------------sreg----------------------------------------------			
		case 's'://sreg
			if (o_pCommand->m_objSpecialFields.m_nFf!=-128){
				nExtensionLength=o_pCommand->m_objSpecialFields.m_nFf;
			}else{
				if (o_pCommand->m_objSpecialFields.m_nUuu!=-128){
					nExtensionLength=o_pCommand->m_objSpecialFields.m_nUuu;
				}else{
					fseek(hOpcodeFile,nCommandOffset+MODRM_FIELD_OFFSET+3,SEEK_SET);//fp on mod/opcode section
					fread(&nBufferByte,1,1,hOpcodeFile);
					if (nBufferByte=='s'){
						nExtensionLength=o_pCommand->m_objModrm.m_nReg;
						//printf("\t\tDEBUG\n");
					}else{//sreg in rm section
						nExtensionLength=o_pCommand->m_objModrm.m_nRm;
						//printf("\t\tDEBUG\n");
					}
				}
			}
			//in ext_len number of register
			switch(nExtensionLength){
			case 0://es
				o_pCommand->m_pParameters[idxOperand][0]='e';
				o_pCommand->m_pParameters[idxOperand][1]='s';
				o_pCommand->m_pParameters[idxOperand][2]=0;
				break;
				
			case 1://cs
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='s';
				o_pCommand->m_pParameters[idxOperand][2]=0;
				break;
				
			case 2://ss
				o_pCommand->m_pParameters[idxOperand][0]='s';
				o_pCommand->m_pParameters[idxOperand][1]='s';
				o_pCommand->m_pParameters[idxOperand][2]=0;
				break;

			case 3://ds
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='s';
				o_pCommand->m_pParameters[idxOperand][2]=0;
				break;

			case 4://fs
				o_pCommand->m_pParameters[idxOperand][0]='f';
				o_pCommand->m_pParameters[idxOperand][1]='s';
				o_pCommand->m_pParameters[idxOperand][2]=0;
				break;						
				
			case 5://gs
				o_pCommand->m_pParameters[idxOperand][0]='g';
				o_pCommand->m_pParameters[idxOperand][1]='s';
				o_pCommand->m_pParameters[idxOperand][2]=0;
				break;
				
			case 6://reserved1
				fclose(hOpcodeFile);
				free(pBytes);
				return RET_ERROR;//error
				break;
				
			case 7://reserved2
				return RET_ERROR;//error
				fclose(hOpcodeFile);
				free(pBytes);
				break;
			}
			break;
			
			//---------------------------------------------------- 1 ------------------------------------------------------			
		case '1':
			o_pCommand->m_pParameters[idxOperand][0]='1';
			o_pCommand->m_pParameters[idxOperand][1]=0;
			break;
			
			//-------------------------------------------------al ax eax---------------------------------------------------
		case 'a':
			if (o_pCommand->m_objSpecialFields.m_nW==0){//al
				o_pCommand->m_pParameters[idxOperand][0]='a';
				o_pCommand->m_pParameters[idxOperand][1]='l';
				o_pCommand->m_pParameters[idxOperand][2]=0;
			}else{
				if (o_pCommand->m_objPrefix.m_nOpr==0x66){//ax
					o_pCommand->m_pParameters[idxOperand][0]='a';
					o_pCommand->m_pParameters[idxOperand][1]='x';
					o_pCommand->m_pParameters[idxOperand][2]=0;
				}else{//eax
					o_pCommand->m_pParameters[idxOperand][0]='e';
					o_pCommand->m_pParameters[idxOperand][1]='a';
					o_pCommand->m_pParameters[idxOperand][2]='x';
					o_pCommand->m_pParameters[idxOperand][3]=0;
				}
			}
			break;
			//------------------------------------------------- cl ---------------------------------------------------			
		case 'b'://cl
			o_pCommand->m_pParameters[idxOperand][0]='c';
			o_pCommand->m_pParameters[idxOperand][1]='l';
			o_pCommand->m_pParameters[idxOperand][2]=0;
			break;
			
			//------------------------------------------------- dr0-7 ---------------------------------------------------
		case 'd'://dr0-7		
			if (o_pCommand->m_objSpecialFields.m_nEee!=-128){
				nExtensionLength=o_pCommand->m_objSpecialFields.m_nEee;
			}else{
				fseek(hOpcodeFile,MODRM_FIELD_OFFSET+3,SEEK_SET);
				fread(&nBufferByte,1,1,hOpcodeFile);
				if (nBufferByte=='e'){//eee
					nExtensionLength=o_pCommand->m_objModrm.m_nReg;
				}else{//in rm field
					nExtensionLength=o_pCommand->m_objModrm.m_nRm;
				}
			}
			switch(nExtensionLength){
			case 0://dr0
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='0';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 1://dr1
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='1';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 2://dr2
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='2';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;

			case 3://dr3
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='3';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;

			case 4://dr4
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='4';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;						
				
			case 5://dr5
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='5';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 6://dr6
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='6';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 7://dr7
				o_pCommand->m_pParameters[idxOperand][0]='d';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='7';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
			}
			break;
			//------------------------------------------------- cr0-7 ---------------------------------------------------			
		case 'c'://cr0-7
#ifdef _DEBUG
			printf("\toperand1:cr0-7\n");
#endif
			if (o_pCommand->m_objSpecialFields.m_nEee!=-128){
				nExtensionLength=o_pCommand->m_objSpecialFields.m_nEee;
			}else{
				fseek(hOpcodeFile,MODRM_FIELD_OFFSET+3,SEEK_SET);
				fread(&nBufferByte,1,1,hOpcodeFile);
				if (nBufferByte=='e'){//eee
					nExtensionLength=o_pCommand->m_objModrm.m_nReg;
				}else{//in rm field
					nExtensionLength=o_pCommand->m_objModrm.m_nRm;
				}
			}
			switch(nExtensionLength){
			case 0://cr0
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='0';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 1://cr1
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='1';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 2://cr2
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='2';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;

			case 3://cr3
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='3';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;

			case 4://cr4
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='4';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;						
				
			case 5://cr5
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='5';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 6://cr6
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='6';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
				
			case 7://cr7
				o_pCommand->m_pParameters[idxOperand][0]='c';
				o_pCommand->m_pParameters[idxOperand][1]='r';
				o_pCommand->m_pParameters[idxOperand][2]='7';
				o_pCommand->m_pParameters[idxOperand][3]=0;
				break;
			}
			break;
			
			//------------------------------------------------- memory ---------------------------------------------------				
		case 'm'://0x67, 0x66, segment redefinition prefixes affects
			if (o_pCommand->m_objSpecialFields.m_nW==0){//1 byte operand
				o_pCommand->m_pParameters[idxOperand][0]='B';
				o_pCommand->m_pParameters[idxOperand][1]='Y';
				o_pCommand->m_pParameters[idxOperand][2]='T';
				o_pCommand->m_pParameters[idxOperand][3]='E';
				nCommandLength=4;
			}else{//2|4 byte operand 
				if (o_pCommand->m_objPrefix.m_nOpr==0x66){//0x66 -2byte
					o_pCommand->m_pParameters[idxOperand][0]='W';
					o_pCommand->m_pParameters[idxOperand][1]='O';
					o_pCommand->m_pParameters[idxOperand][2]='R';
					o_pCommand->m_pParameters[idxOperand][3]='D';
					nCommandLength=4;
				}else{//no 0x66 -4byte
					o_pCommand->m_pParameters[idxOperand][0]='D';
					o_pCommand->m_pParameters[idxOperand][1]='W';
					o_pCommand->m_pParameters[idxOperand][2]='O';
					o_pCommand->m_pParameters[idxOperand][3]='R';
					o_pCommand->m_pParameters[idxOperand][4]='D';
					nCommandLength=5;
				}
			}

			o_pCommand->m_pParameters[idxOperand][nCommandLength]='_';
			o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='P';
			o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='T';
			o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='R';
			o_pCommand->m_pParameters[idxOperand][nCommandLength+4]='_';
			nCommandLength+=5;

			switch(o_pCommand->m_objPrefix.m_nSegr){
			case 0x2e: //cs
				o_pCommand->m_pParameters[idxOperand][nCommandLength]='C';
				break;
			case 0x36://ss
				o_pCommand->m_pParameters[idxOperand][nCommandLength]='S';
				
				break;
			case 0x26: //es
				o_pCommand->m_pParameters[idxOperand][nCommandLength]='E';
				
				break;
			case 0x64: //fs
				o_pCommand->m_pParameters[idxOperand][nCommandLength]='C';
				
				break;
			case 0x65: //gs
				o_pCommand->m_pParameters[idxOperand][nCommandLength]='G';
				break;
			default: //ds
				o_pCommand->m_pParameters[idxOperand][nCommandLength]='D';
				break;
			}

			o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='S';
			o_pCommand->m_pParameters[idxOperand][nCommandLength+2]=':';
			o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='[';
			nCommandLength+=3;

			if (o_pCommand->m_objPrefix.m_nSr!=0x67){//0x67 not present
				o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='e';
				switch(o_pCommand->m_objModrm.m_nRm){
				case 0://eax
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='a';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
					nBufferByte=4;
					break;
					
				case 1://ecx
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='c';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
					nBufferByte=4;
					break;
					
				case 2://edx
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='d';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
					nBufferByte=4;
					break;
					
				case 3://ebx
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='b';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
					nBufferByte=4;
					break;
					
				case 4://SIB
					o_pCommand->m_objSib.m_nScale=pBytes[idxByte]/64;
					o_pCommand->m_objSib.m_nIndex=pBytes[idxByte]/8-(pBytes[idxByte]/64)*8;
					o_pCommand->m_objSib.m_nBase=pBytes[idxByte]%8;
					o_pCommand->m_objSib.m_nSib=pBytes[idxByte];
					++idxByte;//SIB parsed

					//cp->parameters[counter][1]='e';
					switch(o_pCommand->m_objSib.m_nBase){
					case 0:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='a';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
						break;
						
					case 1:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='c';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
						break;
						
					case 2:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='d';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
						break;
						
					case 3:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='b';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='x';
						break;
						
					case 4:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='s';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='p';
						break;
						
					case 5://[*]
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='b';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='p';
						break;
						
					case 6:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='s';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='i';
						break;
						
					case 7:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='d';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='i';
						break;
					}
					
					o_pCommand->m_pParameters[idxOperand][nCommandLength+4]='+';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+5]='e';
					
					switch(o_pCommand->m_objSib.m_nIndex){
					case 0:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='a';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='x';
						break;
						
					case 1:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='c';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='x';
						break;
						
					case 2:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='d';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='x';
						break;
						
					case 3:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='b';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='x';
						break;
						
					case 4:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='s';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='p';
						break;
						
					case 5://[*]
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='b';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='p';
						break;
						
					case 6:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='s';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='i';
						break;
						
					case 7:
						o_pCommand->m_pParameters[idxOperand][nCommandLength+6]='d';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+7]='i';
						break;
					}
					
					o_pCommand->m_pParameters[idxOperand][nCommandLength+8]='*';
					nExtensionLength=0;
					nBufferByte=1;
					
					while (nExtensionLength < o_pCommand->m_objSib.m_nScale){
						nBufferByte*=2;
						++nExtensionLength;
					}	
					o_pCommand->m_pParameters[idxOperand][nCommandLength+9]=nBufferByte+'0';
					nBufferByte=10;
					break;
					
				case 5://can be displacement only
					if (o_pCommand->m_objModrm.m_nMod==0){
						nExtensionLength=4;
						while (nExtensionLength != 0){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]=pBytes[idxByte]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]-'0')>9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]-'0'-10;
							}
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]=pBytes[idxByte]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]-'0')>9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]-'0'-10;
							}
							++idxByte;
							--nExtensionLength;
						}
						nBufferByte=9;
					}else{
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='b';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='p';
						nBufferByte=4;
					}
					break;
					
				case 6://esi
					//cp->parameters[counter][1]='e';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='s';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='i';
					nBufferByte=4;
					break;
					
				case 7://edi
					//cp->parameters[counter][1]='e';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='d';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='i';
					nBufferByte=4;
					break;	
				}
				nExtensionLength=o_pCommand->m_objModrm.m_nMod*o_pCommand->m_objModrm.m_nMod;//in ext_len number of displacement bytes
				if (nExtensionLength!=0){
					if ((nExtensionLength==1) && (pBytes[idxByte]>0x7f)){//1 byte sign extended
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte]='-';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]=(0x100-pBytes[idxByte])/16+'0';
						if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]-'0')>9){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]-'0'-10;
						}
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]=(0x100-pBytes[idxByte])%16+'0';
						if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]-'0')>9){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]-'0'-10;
						}
					}else{
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte]='+';
						while (nExtensionLength!=0){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]=pBytes[idxByte]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]-'0')>9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]-'0'-10;
							}
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]=pBytes[idxByte]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]-'0')>9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]-'0'-10;
							}
							++idxByte;
							--nExtensionLength;
						}
					}
					nBufferByte=nBufferByte+o_pCommand->m_objModrm.m_nMod*o_pCommand->m_objModrm.m_nMod*2+1;
				}
				o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte]=']';
				o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]=0;
			}else{//0x67 present
				switch (o_pCommand->m_objModrm.m_nRm){
				case 0:
					o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='b';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='x';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='+';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+4]='s';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+5]='i';
					nBufferByte=6;
					break;
					
				case 1:
					o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='b';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='x';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='+';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+4]='d';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+5]='i';
					nBufferByte=6;
					break;
					
				case 2:
					o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='b';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='p';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='+';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+4]='s';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+5]='i';
					nBufferByte=6;
					break;
					
				case 3:
					o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='b';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='p';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+3]='+';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+4]='d';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+5]='i';
					nBufferByte=6;
					break;
					
				case 4:
					o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='s';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='i';
					nBufferByte=3;
					break;
					
				case 5:
					o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='d';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='i';
					nBufferByte=3;
					break;
					
				case 6://can be disp16 only
					if (0 == o_pCommand->m_objModrm.m_nMod ){
						nExtensionLength=2;
						while (nExtensionLength != 0){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]=pBytes[idxByte]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]-'0') > 9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2]-'0'-10;
							}
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]=pBytes[idxByte]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if ((o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]-'0') > 9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nExtensionLength*2-1]-'0'-10;
							}
							++idxByte;
							--nExtensionLength;
						}
						nBufferByte=5;
					}else{
						o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='b';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='p';
						nBufferByte=3;
					}
					break;
					
				case 7:
					o_pCommand->m_pParameters[idxOperand][nCommandLength+1]='b';
					o_pCommand->m_pParameters[idxOperand][nCommandLength+2]='x';
					nBufferByte=3;
					break;
				}
				
				nExtensionLength=o_pCommand->m_objModrm.m_nMod;//in ext_len number of displacement bytes
				if (nExtensionLength!=0){
					
					if ((nExtensionLength==1) && (pBytes[idxByte]>0x7f)){//1 byte sign extended
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte]='-';
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]=(0x100-pBytes[idxByte])/16+'0';
						if ( (o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]-'0') > 9 ){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]-'0'-10;
						}
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]=(0x100-pBytes[idxByte])%16+'0';
						if ( (o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]-'0') > 9 ){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+2]-'0'-10;
						}
					}else{
						
						o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte]='+';
						while (nExtensionLength != 0){
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]=pBytes[idxByte]%16+'0';
							//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
							//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
							if ( (o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]-'0') > 9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2]-'0'-10;
							}
							o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]=pBytes[idxByte]/16+'0';
							//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
							if ( (o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]-'0') > 9){
								o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]='A'+o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+nExtensionLength*2-1]-'0'-10;
							}
							++idxByte;
							--nExtensionLength;
						}
					}
					nBufferByte=nBufferByte+o_pCommand->m_objModrm.m_nMod*2+1;
				}
				o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte]=']';
				o_pCommand->m_pParameters[idxOperand][nCommandLength+nBufferByte+1]=0;
			}

			break;
			
			//-----------------------------------------------immediate--------------------------------------------------	
		case 'i'://immediate //0x66/,s,w (,d) affects
			nExtensionLength=1;
			if (1 == o_pCommand->m_objSpecialFields.m_nS){//1byte imm
				if (pBytes[idxByte] > 0x7F){//-
					o_pCommand->m_pParameters[idxOperand][0]='-';
					o_pCommand->m_pParameters[idxOperand][1]=(0x100-pBytes[idxByte])/16+'0';
					o_pCommand->m_pParameters[idxOperand][2]=(0x100-pBytes[idxByte])%16+'0';
					if ( (o_pCommand->m_pParameters[idxOperand][1]-'0') > 9 ){
						o_pCommand->m_pParameters[idxOperand][1]='A'+o_pCommand->m_pParameters[idxOperand][1]-'0'-10;
					}
					if ( (o_pCommand->m_pParameters[idxOperand][2]-'0') > 9 ){
						o_pCommand->m_pParameters[idxOperand][2]='A'+o_pCommand->m_pParameters[idxOperand][2]-'0'-10;
					}
					o_pCommand->m_pParameters[idxOperand][3]=0;
					++idxByte;
					break;
				}
			}else{
				if (o_pCommand->m_objSpecialFields.m_nW == 1){//2,4 byte imm
					if (o_pCommand->m_objPrefix.m_nOpr == 0x66){//2 bytes imm
						nExtensionLength =2;
					}else{//4 bytes imm
						nExtensionLength=4;
					}
				}
			}

			o_pCommand->m_pParameters[idxOperand][nExtensionLength*2]=0;

			while (nExtensionLength!=0){
				o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-1]=pBytes[idxByte]%16+'0';
				//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
				//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
				if ( (o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-1]-'0') > 9){
					o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-1]='A'+o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-1]-'0'-10;
				}
				o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-2]=pBytes[idxByte]/16+'0';
				//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
				if ( (o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-2]-'0') > 9 ){
					o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-2]='A'+o_pCommand->m_pParameters[idxOperand][nExtensionLength*2-2]-'0'-10;
				}
				++idxByte;
				--nExtensionLength;
			}

			break;
			
			//8bit immediate----------------------------------------------------------------------------------------------------
		case '8':
			o_pCommand->m_pParameters[idxOperand][0]=pBytes[idxByte]/16+'0';
			if ( (o_pCommand->m_pParameters[idxOperand][0]-'0') > 9 ){
				o_pCommand->m_pParameters[idxOperand][0]='A'+o_pCommand->m_pParameters[idxOperand][0]-'0'-10;
			}
			o_pCommand->m_pParameters[idxOperand][1]=pBytes[idxByte]%16+'0';
			if ( (o_pCommand->m_pParameters[idxOperand][1]-'0') > 9 ){
				o_pCommand->m_pParameters[idxOperand][1]='A'+o_pCommand->m_pParameters[idxOperand][1]-'0'-10;
			}
			o_pCommand->m_pParameters[idxOperand][2]=0;
			++idxByte;
			break;
			
			//16bit immediate----------------------------------------------------------------------------------------------------
		case '6':
			nBufferByte=2;
			while (nBufferByte != 0){//0,1
				nBufferByte--;
				o_pCommand->m_pParameters[idxOperand][nBufferByte*2]=pBytes[idxByte]/16+'0';
				if ( (o_pCommand->m_pParameters[idxOperand][nBufferByte*2]-'0') > 9){
					o_pCommand->m_pParameters[idxOperand][nBufferByte*2]='A'+o_pCommand->m_pParameters[idxOperand][0]-'0'-10;
				}
				o_pCommand->m_pParameters[idxOperand][nBufferByte*2+1]=pBytes[idxByte]%16+'0';
				if ( (o_pCommand->m_pParameters[idxOperand][nBufferByte*2+1]-'0') > 9 ){
					o_pCommand->m_pParameters[idxOperand][nBufferByte*2+1]='A'+o_pCommand->m_pParameters[idxOperand][nBufferByte*2+1]-'0'-10;
				}
				++idxByte;
			}
			o_pCommand->m_pParameters[idxOperand][4]=0;
			break;
			
			//dx(ex. variable port)----------------------------------------------------------------------------------------------------
		case 'v':
			o_pCommand->m_pParameters[idxOperand][0]='d';
			o_pCommand->m_pParameters[idxOperand][1]='x';
			o_pCommand->m_pParameters[idxOperand][2]=0;
			break;
			//tttn-----------------------------------------------------------------------------------------------------------------------------------
		case 't'://tttn
			o_pCommand->m_nParameterCount--;//tttn is modification, not parameter
			nExtensionLength=0;
			while (0 != o_pCommand->m_strCommandText[nExtensionLength]){
				++nExtensionLength;
			}
			o_pCommand->m_strCommandText=(char*)realloc( o_pCommand->m_strCommandText,nExtensionLength+3);//????????? ??? ??????//command + 2 chars + 0
			//in ext_len position of zero in com_text
			switch(o_pCommand->m_objSpecialFields.m_nTttn){
			case 0://overflow
				o_pCommand->m_strCommandText[nExtensionLength]='o';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;
				
			case 1://no overflow
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='o';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;
				
			case 2://below
				o_pCommand->m_strCommandText[nExtensionLength]='b';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;

			case 3://not below
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='b';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;

			case 4://zero
				o_pCommand->m_strCommandText[nExtensionLength]='z';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;						
				
			case 5://not zero
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='z';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;
				
			case 6://not above
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='a';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;
				
			case 7://above
				o_pCommand->m_strCommandText[nExtensionLength]='a';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;
				
			case 8://sign
				o_pCommand->m_strCommandText[nExtensionLength]='s';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;
				
			case 9://no sign
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='s';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;
				
			case 10://parity
				o_pCommand->m_strCommandText[nExtensionLength]='p';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;

			case 11://not parity
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='p';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;

			case 12://less
				o_pCommand->m_strCommandText[nExtensionLength]='l';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;						
				
			case 13://not less
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='l';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;
				
			case 14://not greater
				o_pCommand->m_strCommandText[nExtensionLength]='n';
				o_pCommand->m_strCommandText[nExtensionLength+1]='g';
				o_pCommand->m_strCommandText[nExtensionLength+2]=0;
				break;
				
			case 15://greater
				o_pCommand->m_strCommandText[nExtensionLength]='g';
				o_pCommand->m_strCommandText[nExtensionLength+1]=0;
				break;
			}
			break;
			
			//f: pushf -/w/d, pusha -/w/d, popf -/w/d, popa -/w/d: modification---------------------------------------------------------------------------------------
		case 'f'://pushf -/w/d, pusha -/w/d, popf -/w/d, popa -/w/d: modification
			o_pCommand->m_nParameterCount--;//modification, not parameter
			nExtensionLength=0;
			while (o_pCommand->m_strCommandText[nExtensionLength]!=0){
				++nExtensionLength;
			}
			o_pCommand->m_strCommandText=(char*)realloc( o_pCommand->m_strCommandText,nExtensionLength+2);//????????? ??? ??????//command + 1 char + 0
			//in ext_len position of zero in com_text
			if (o_pCommand->m_objPrefix.m_nOpr==0x66){//0x66
				o_pCommand->m_strCommandText[nExtensionLength]='w';
			}else{//no 0x66
				o_pCommand->m_strCommandText[nExtensionLength]='d';
			}
			o_pCommand->m_strCommandText[nExtensionLength+1]=0;
			break;
			
			//j:jcxz/jecxz: modification---------------------------------------------------------------------------------------
		case 'j'://jcxz/jecxz: modification
			o_pCommand->m_nParameterCount--;//modification, not parameter
			nExtensionLength=1;
			o_pCommand->m_strCommandText=(char*)realloc( o_pCommand->m_strCommandText,6);//????????? ??? ??????//'jecxz' + 0

			if (o_pCommand->m_objPrefix.m_nSr!=0x67){//0x66
				o_pCommand->m_strCommandText[1]='e';
				++nExtensionLength;
			}
			o_pCommand->m_strCommandText[nExtensionLength]='c';
			o_pCommand->m_strCommandText[nExtensionLength+1]='x';
			o_pCommand->m_strCommandText[nExtensionLength+2]='z';
			o_pCommand->m_strCommandText[nExtensionLength+3]=0;
			break;
			
			//':'-DWORD PTR ds:[esi]-DWORD PTR es:[edi]------------------------------------------------------------------------------------
		case ':'://DWORD PTR ds:[esi]-DWORD PTR es:[edi]
			if (o_pCommand->m_objSpecialFields.m_nW==0){//1 byte operand
				o_pCommand->m_pParameters[idxOperand][0]='B';
				o_pCommand->m_pParameters[idxOperand][1]='Y';
				o_pCommand->m_pParameters[idxOperand][2]='T';
				o_pCommand->m_pParameters[idxOperand][3]='E';
				nExtensionLength=4;
			}else{//2|4 byte operand 
				if (o_pCommand->m_objPrefix.m_nOpr==0x66){//0x66 -2byte
					o_pCommand->m_pParameters[idxOperand][0]='W';
					o_pCommand->m_pParameters[idxOperand][1]='O';
					o_pCommand->m_pParameters[idxOperand][2]='R';
					o_pCommand->m_pParameters[idxOperand][3]='D';
					nExtensionLength=4;
				}else{//no 0x66 -4byte
					o_pCommand->m_pParameters[idxOperand][0]='D';
					o_pCommand->m_pParameters[idxOperand][1]='W';
					o_pCommand->m_pParameters[idxOperand][2]='O';
					o_pCommand->m_pParameters[idxOperand][3]='R';
					o_pCommand->m_pParameters[idxOperand][4]='D';
					nExtensionLength=5;
				}
			}

			o_pCommand->m_pParameters[idxOperand][nExtensionLength]='_';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+1]='P';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+2]='T';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+3]='R';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+4]='_';
			nExtensionLength+=5;

			if (0==idxOperand){//1st par
				
				switch (o_pCommand->m_objPrefix.m_nSegr){
				case 0x2e: //cs
					o_pCommand->m_pParameters[idxOperand][nExtensionLength]='C';
					break;
				case 0x36://ss
					o_pCommand->m_pParameters[idxOperand][nExtensionLength]='S';
					
					break;
				case 0x26: //es
					o_pCommand->m_pParameters[idxOperand][nExtensionLength]='E';
					
					break;
				case 0x64: //fs
					o_pCommand->m_pParameters[idxOperand][nExtensionLength]='C';
					
					break;
				case 0x65: //gs
					o_pCommand->m_pParameters[idxOperand][nExtensionLength]='G';
					break;
				default:
					o_pCommand->m_pParameters[idxOperand][nExtensionLength]='D';
					break;
				}
			}else{
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='E';
			}
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+1]='S';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+2]=':';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+3]='[';
			nExtensionLength+=4;
			if (o_pCommand->m_objPrefix.m_nSr!=0x67){//no 0x67-4byte addr
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='E';
				++nExtensionLength;
			}
			if (idxOperand==0){//1st par
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='S';
			}else{
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='D';
			}
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+1]='I';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+2]=']';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+3]=0;

			break;
			
			//'g'-JMP FAR, CALL FAR------------------------------------------------------------------------------------
		case 'g'://JMP FAR, CALL FAR
			nBufferByte=255;
			nExtensionLength=0;
			o_pCommand->m_pParameters[idxOperand][0]='F';
			o_pCommand->m_pParameters[idxOperand][1]='A';
			o_pCommand->m_pParameters[idxOperand][2]='R';
			o_pCommand->m_pParameters[idxOperand][3]='_';
			o_pCommand->m_pParameters[idxOperand]+=4;
			if (o_pCommand->m_objPrefix.m_nOpr==0x66){//0x67 present-4433:2211
				nBufferByte2=4;
			}else{//6655:44332211
				nBufferByte2=6;
			}
			while (nExtensionLength!=(int)nBufferByte2){
				o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2-1]=pBytes[idxByte]/16+'0';
				o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2]=pBytes[idxByte]%16+'0';
				if ((o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2-1]-'0')>9){
					o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2-1]='A'+o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2-1]-'0'-10;
				}
				if ((o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2]-'0')>9){
					o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2]='A'+o_pCommand->m_pParameters[idxOperand][nBufferByte-255+(nBufferByte2-nExtensionLength)*2]-'0'-10;
				}
				++idxByte;
				++nExtensionLength;
				if ((nBufferByte2-nExtensionLength)==2){
					o_pCommand->m_pParameters[idxOperand][4]=':';
					nBufferByte=254;
				}
			}
			o_pCommand->m_pParameters[idxOperand][nBufferByte2*2+1]=0;
			o_pCommand->m_pParameters[idxOperand]-=4;
			break;
			
			//'x'-xlat: byte ptr ds[ebx/bx+al]------------------------------------------------------------------------------------
		case 'x'://xlat
			nExtensionLength=0;
			o_pCommand->m_pParameters[idxOperand][0]='B';
			o_pCommand->m_pParameters[idxOperand][1]='Y';
			o_pCommand->m_pParameters[idxOperand][2]='T';
			o_pCommand->m_pParameters[idxOperand][3]='E';
			o_pCommand->m_pParameters[idxOperand][4]='_';
			o_pCommand->m_pParameters[idxOperand][5]='P';
			o_pCommand->m_pParameters[idxOperand][6]='T';
			o_pCommand->m_pParameters[idxOperand][7]='R';
			o_pCommand->m_pParameters[idxOperand][8]='_';

			o_pCommand->m_pParameters[idxOperand][9]='D';

			switch(o_pCommand->m_objPrefix.m_nSegr){
			case 0x2e:
				o_pCommand->m_pParameters[idxOperand][9]='C';
				break;
			case 0x36:
				o_pCommand->m_pParameters[idxOperand][9]='S';
				break;
			case 0x26:
				o_pCommand->m_pParameters[idxOperand][9]='E';
				break;
			case 0x64:
				o_pCommand->m_pParameters[idxOperand][9]='F';
				break;
			case 0x65:
				o_pCommand->m_pParameters[idxOperand][9]='G';
				break;
			}

			o_pCommand->m_pParameters[idxOperand][10]='S';
			o_pCommand->m_pParameters[idxOperand][11]=':';
			o_pCommand->m_pParameters[idxOperand][12]='[';

			if (o_pCommand->m_objPrefix.m_nSr!=0x67){//ebx
				o_pCommand->m_pParameters[idxOperand][13]='E';
				nExtensionLength=1;
			}

			o_pCommand->m_pParameters[idxOperand][13+nExtensionLength]='B';
			o_pCommand->m_pParameters[idxOperand][14+nExtensionLength]='X';
			o_pCommand->m_pParameters[idxOperand][15+nExtensionLength]='+';
			o_pCommand->m_pParameters[idxOperand][16+nExtensionLength]='A';
			o_pCommand->m_pParameters[idxOperand][17+nExtensionLength]='L';
			o_pCommand->m_pParameters[idxOperand][18+nExtensionLength]=']';
			o_pCommand->m_pParameters[idxOperand][19+nExtensionLength]=0;
			break;
			
			//'q':rep/repnz влияют на описание команды: modification-------------------------------------------------------------------------------------
		case 'q'://q
			o_pCommand->m_nParameterCount--;
			nExtensionLength=strlen(o_pCommand->m_strCommandText);
			if (o_pCommand->m_objPrefix.m_nLnr==0xf2){//repnz
				strTempText=(char*)malloc(nExtensionLength+7);//'repne ',0
				strTempText[0]='r'; //TODO strcpy
				strTempText[1]='e';
				strTempText[2]='p';
				strTempText[3]='n';
				strTempText[4]='z';
				strTempText[5]='_';
				strTempText[nExtensionLength+7]=0;
				nExtensionLength=6;
				
			}
			if (o_pCommand->m_objPrefix.m_nLnr==0xf3){//rep
				strTempText=(char*)malloc(nExtensionLength+5);//'rep ',0
				strTempText[0]='r';//TODO strcpy
				strTempText[1]='e';
				strTempText[2]='p';
				strTempText[3]='_';
				strTempText[nExtensionLength+5]=0;
				nExtensionLength=4;
			}

			if (o_pCommand->m_objPrefix.m_nLnr==0xff){//no rep/repne
				break;
			}

			strcpy(strTempText+nExtensionLength,o_pCommand->m_strCommandText);//now command description modified
			free(o_pCommand->m_strCommandText);
			o_pCommand->m_strCommandText=strTempText;
			break;
			
			//'u'-es:[edi](ins/ous)------------------------------------------------------------------------------------
		case 'u':
			if (o_pCommand->m_objSpecialFields.m_nW==0){//1 byte operand
				o_pCommand->m_pParameters[idxOperand][0]='B';//TODO strcpy
				o_pCommand->m_pParameters[idxOperand][1]='Y';
				o_pCommand->m_pParameters[idxOperand][2]='T';
				o_pCommand->m_pParameters[idxOperand][3]='E';
				nExtensionLength=4;
			}else{//2|4 byte operand 
				if (o_pCommand->m_objPrefix.m_nOpr==0x66){//0x66 -2byte
					o_pCommand->m_pParameters[idxOperand][0]='W';//TODO strcpy
					o_pCommand->m_pParameters[idxOperand][1]='O';
					o_pCommand->m_pParameters[idxOperand][2]='R';
					o_pCommand->m_pParameters[idxOperand][3]='D';
					nExtensionLength=4;
				}else{//no 0x66 -4byte
					o_pCommand->m_pParameters[idxOperand][0]='D';//TODO strcpy
					o_pCommand->m_pParameters[idxOperand][1]='W';
					o_pCommand->m_pParameters[idxOperand][2]='O';
					o_pCommand->m_pParameters[idxOperand][3]='R';
					o_pCommand->m_pParameters[idxOperand][4]='D';
					nExtensionLength=5;
				}
			}

			o_pCommand->m_pParameters[idxOperand][nExtensionLength]='_';//TODO strcpy
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+1]='P';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+2]='T';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+3]='R';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+4]='_';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+5]='E';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+6]='S';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+7]=':';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+8]='[';
			nExtensionLength+=9;
			if (o_pCommand->m_objPrefix.m_nSr!=0x67){//edi
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='E';
				++nExtensionLength;
			}
			
			o_pCommand->m_pParameters[idxOperand][nExtensionLength]='D';
			
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+1]='I';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+2]=']';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+3]=0;

			break;
			
			//-----------------------------------------------imm as memory (mov eax,full displacement)--------------------------------------------------	
		case 'h'://imm as memory (mov eax,full displacement)//0x66/0x67/segment redefinition/w/d affects

			if (o_pCommand->m_objSpecialFields.m_nW==0){//1 byte operand
				o_pCommand->m_pParameters[idxOperand][0]='B';//TODO strcpy
				o_pCommand->m_pParameters[idxOperand][1]='Y';
				o_pCommand->m_pParameters[idxOperand][2]='T';
				o_pCommand->m_pParameters[idxOperand][3]='E';
				nExtensionLength=4;
			}else{//2|4 byte operand 
				if (o_pCommand->m_objPrefix.m_nOpr==0x66){//0x66 -2byte
					o_pCommand->m_pParameters[idxOperand][0]='W';//TODO strcpy
					o_pCommand->m_pParameters[idxOperand][1]='O';
					o_pCommand->m_pParameters[idxOperand][2]='R';
					o_pCommand->m_pParameters[idxOperand][3]='D';
					nExtensionLength=4;
				}else{//no 0x66 -4byte
					o_pCommand->m_pParameters[idxOperand][0]='D';//TODO strcpy
					o_pCommand->m_pParameters[idxOperand][1]='W';
					o_pCommand->m_pParameters[idxOperand][2]='O';
					o_pCommand->m_pParameters[idxOperand][3]='R';
					o_pCommand->m_pParameters[idxOperand][4]='D';
					nExtensionLength=5;
				}
			}

			o_pCommand->m_pParameters[idxOperand][nExtensionLength]='_';//TODO strcpy
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+1]='P';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+2]='T';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+3]='R';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+4]='_';
			nExtensionLength+=5;

			switch(o_pCommand->m_objPrefix.m_nSegr){
			case 0x2e: //cs
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='C';
				break;
			case 0x36://ss
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='S';
				break;
			case 0x26: //es
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='E';
				break;
			case 0x64: //fs
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='C';
				break;
			case 0x65: //gs
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='G';
				break;
			default:
				o_pCommand->m_pParameters[idxOperand][nExtensionLength]='D';
				break;
			}

			o_pCommand->m_pParameters[idxOperand][nExtensionLength+1]='S';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+2]=':';
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+3]='[';
			nExtensionLength+=4;

			if (o_pCommand->m_objPrefix.m_nSr==0x67){//2 bytes imm
				nBufferByte =2;
			}else{//4 bytes imm
				nBufferByte=4;
			}

			o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2+1]=0;
			o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2]=']';

			while (nBufferByte!=0){
				o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-1]=pBytes[idxByte]%16+'0';
				//printf("\tmp[bytecounter]:%d\n",mp[bytecounter]);
				//printf("\tcp->parameters[counter][4+ext_len*2]:%d\n",cp->parameters[counter][4+ext_len*2]);
				if ((o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-1]-'0')>9){
					o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-1]='A'+o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-1]-'0'-10;
				}
				o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-2]=pBytes[idxByte]/16+'0';
				//printf("\tcp->parameters[counter][4+ext_len*2-1]:%d\n",cp->parameters[counter][4+ext_len*2-1]);
				if ((o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-2]-'0')>9){
					o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-2]='A'+o_pCommand->m_pParameters[idxOperand][nExtensionLength+nBufferByte*2-2]-'0'-10;
				}
				++idxByte;
				--nBufferByte;
			}

			break;
			
			//-------------------------------------------------Call/jmp far intersegment — memory operand redefinition---------------------------------------------------				
case 'k'://Call/jmp far intersegment
o_pCommand->m_nParameterCount--;
nBufferByte=0;//if 0x66 is present
if (o_pCommand->m_objPrefix.m_nOpr==0x66){
	nBufferByte=1;
}
nExtensionLength=5+nBufferByte+strlen(o_pCommand->m_pParameters[0]);//'FAR '+ strlen(cp->parameters[0]),0
strTempText=(char*)malloc(nExtensionLength);
strTempText[nExtensionLength-1]=0;
strTempText[0]='F';//TODO strcpy
strTempText[1]='A';
strTempText[2]='R';
strTempText[3]='_';
if (o_pCommand->m_objPrefix.m_nOpr==0x66){
	strTempText[4]='D';//dword
}else{
	strTempText[4]='F';//fword
}
strcpy(strTempText+5,o_pCommand->m_pParameters[0]+1-nBufferByte);
//printf("\nTEMP_CHAR1=%s\n",temp_char1);
free(o_pCommand->m_pParameters[0]);
o_pCommand->m_pParameters[0]=strTempText;
break;
			
		}
		++idxOperand;
	}
	//-----------------------------------------------operands parsing----------------------------------------------------------------------------------------------------------------------------------------------------------	
	
	fseek(i_hTargetFile,ftell(i_hTargetFile)-nBytesRead+idxByte,SEEK_SET);
	fclose(hOpcodeFile);
	return RET_SUCCESS;
}

//====================================================================================================
IMAGE_SECTION_HEADER findCodeSection(FILE* i_hTargetFile)
{
	//TODO findCodeSection should return code section file offset and file size, not IMAGE_SECTION_HEADER
	//Search code section by AddressOfEntryPoint value
	IMAGE_SECTION_HEADER objSection = { 0 };
	DWORD nPeHeaderOffset;
	WORD nOptionalHeaderSize;
	WORD nSectionCount;
	DWORD nEntryPointRva;
	int idx = 0;
	fseek(i_hTargetFile, 0x3c, SEEK_SET);
	fread(&nPeHeaderOffset, 4, 1, i_hTargetFile); //'PE' offset in int_buf
	//printf("e_lfanew=%#x\n",int_buf);
	fseek(i_hTargetFile, nPeHeaderOffset + 6, SEEK_SET);//fp on NumberOfSections
	fread(&nSectionCount, 2, 1, i_hTargetFile);
	//int_buf=int_buf+sizeof(IMAGE_FILE_HEADER);//fp on SizeOfOptionalHeader-int_buf+sizeof(IMAGE_FILE_HEADER)+4/*'PE'*/-4/*2 positions back in IMAGE_FILE_HEADER*/
	fseek(i_hTargetFile, nPeHeaderOffset + sizeof(IMAGE_FILE_HEADER), SEEK_SET);
	fread(&nOptionalHeaderSize, 2, 1, i_hTargetFile);//SizeOfOptionalHeader in word_buf
	printf("SizeOfOptionalHeader=0x%04x\n", nOptionalHeaderSize);
	fseek(i_hTargetFile, 18, SEEK_CUR);//fp on AddressOfEntryPoint
	fread(&nEntryPointRva, 4, 1, i_hTargetFile);
	printf("AddressOfEntryPoint=0x%08x\n", nEntryPointRva);
	printf("NumberOfSections=0x%04x\n", nSectionCount);
	fseek(i_hTargetFile, nOptionalHeaderSize - 20, SEEK_CUR);//fp on 1st section
	while (idx != nSectionCount) {
		fread(&objSection, sizeof(IMAGE_SECTION_HEADER), 1, i_hTargetFile);
		if ((nEntryPointRva >= objSection.VirtualAddress) && (nEntryPointRva < (objSection.VirtualAddress + objSection.Misc.VirtualSize))) {
			return objSection;
		}
		++idx;
	}
	printf("[ERROR] Failed to find code section.\n");
	return objSection;
}

//====================================================================================================
static int disassembleFile(FILE* i_hTargetFile, FILE* o_hDisassemblyFile)
{
	SCommand objCommand;
	IMAGE_SECTION_HEADER objSection;
	int idxCommandParameter;
	int nRes = RET_SUCCESS;
	long nSectionEndOffset;
	long nFilePosition;
	int nAvailableBytes;

	objSection = findCodeSection(i_hTargetFile);
	fseek(i_hTargetFile, objSection.PointerToRawData, SEEK_SET);
	nSectionEndOffset = (long)objSection.PointerToRawData + objSection.SizeOfRawData;
	
	while ((nFilePosition = ftell(i_hTargetFile)) >= 0 &&
		nFilePosition < nSectionEndOffset) {
		nAvailableBytes = (nSectionEndOffset - nFilePosition > MAX_COMMAND_LEN) ? MAX_COMMAND_LEN : (int)(nSectionEndOffset - nFilePosition);

		SCommand_Init(&objCommand);
		nRes = SCommand_GetCommand(&objCommand, i_hTargetFile, nAvailableBytes);
		if (nRes != RET_SUCCESS) {
			nRes = 1;
			break;
		}

		if (fprintf(o_hDisassemblyFile, "%s", objCommand.m_strCommandText) < 0) {
			nRes = ERR_FILE_OPERATION;
			break;
		}

		idxCommandParameter = 0;
		while (idxCommandParameter < objCommand.m_nParameterCount) {
			if (fprintf(o_hDisassemblyFile, (idxCommandParameter) ? ", %s" : " %s", objCommand.m_pParameters[idxCommandParameter]) < 0) {
				nRes = ERR_FILE_OPERATION;
				goto LABEL_PARSING_LOOP_END;
			}

			++idxCommandParameter;
		}

		if (fprintf(o_hDisassemblyFile, "\n") < 0) {
			nRes = ERR_FILE_OPERATION;
			break;
		}

		SCommand_Destroy(&objCommand);
	}

LABEL_PARSING_LOOP_END:
	if (RET_SUCCESS!=nRes) {
		SCommand_Destroy(&objCommand);
	}

	if (nFilePosition < 0 || (nFilePosition < nSectionEndOffset &&
		nRes != RET_SUCCESS)) {
		fprintf(stderr, "[ERROR] Failed to disassemble target PE file.\n");
		return ERR_FILE_OPERATION;
	}

	return RET_SUCCESS;
}

//====================================================================================================
int main(int i_nArgc, char* i_ppArgv[])
{
	FILE* hTargetFile;
	FILE* hDisassemblyFile = stdout;
	int nResult;
	
	// Parse cmd args
	if (i_nArgc < 2 || i_nArgc > 3) {
		printf("Usage: %s <target pe file path> [output disassembly txt file]\n",
			i_nArgc > 0 ? i_ppArgv[0] : "DisAsm");
		return ERR_INVALID_ARGS;
	}

	if ((hTargetFile = fopen(i_ppArgv[1], "rb")) == NULL) {
		printf("[ERROR] opening target PE file failed: %s\n", i_ppArgv[1]);
		return ERR_FILE_OPERATION;
	}

	if (i_nArgc == 3) {
		hDisassemblyFile = fopen(i_ppArgv[2], "w");
		if (hDisassemblyFile == NULL) {
			printf("[ERROR] opening output file failed : %s\n", i_ppArgv[2]);
			fclose(hTargetFile);
			return ERR_FILE_OPERATION;
		}
	}

	// Perform disassembling
	nResult = disassembleFile(hTargetFile, hDisassemblyFile);

	// Cleanup
	fclose(hTargetFile);
	if (hDisassemblyFile != stdout &&
		fclose(hDisassemblyFile) != 0) {
		printf("[ERROR] writing output file failed: %s\n", i_ppArgv[2]);
		return ERR_FILE_OPERATION;
	}
	
	return nResult;
}
