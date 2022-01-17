/* 
 * 大作业:CPU模拟器
 * @2020 xyToki 2020.4
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/* 运行常量 */
#define INPUT_NAME "dict.dic"
#define MEM_SIZE   32768
#define IR_SIZE        2
#define REG_SUM        8
#define OBJ_LEN       16    //指令中8位2个对象，一个对象4位
#define CODE_GROUP     4	//代码输出的字节组长度
#define DATA_GROUP     2	//数据输出的字节组长度
#define OUT_LINES     16	//输出行数
#define CODE_COUNT     8	//代码每行输出个数
#define DATA_COUNT    16	//数据每行输出个数
#define LINE_LENGTH   34    //输入文件单行读取
#define COMBINE_INT    4    //合并Int
#define COMBINE_SHORT  2    //合并Short
#define JMP_ALL        0    //无条件跳转
#define JMP_ZERO       1    //零跳转
#define JMP_POS        2    //-1跳转
#define JMP_NEG        3    //1跳转
/* 命令常量 */
#define CMD_STOP  0
#define CMD_COPY  1
#define CMD_ADD   2
#define CMD_SUB   3
#define CMD_MUL   4
#define CMD_DIV   5
#define CMD_AND   6
#define CMD_OR    7
#define CMD_NOT   8
#define CMD_CMP   9
#define CMD_GOTO 10
#define CMD_IN   11
#define CMD_OUT  12
typedef struct CPU {          //CPU结构体
    short IP;                 //程序计数器
    short NR;                 //立即数暂存
    short FLAG;               //标志寄存器
    short REG[REG_SUM] = {0}; //1-8 这里放一起
    char  IR [IR_SIZE] = {0}; //指令寄存器
    char  MEM[MEM_SIZE]= {0}; //内存
} CPU;
/* 合并n个char为int或short   */ int   combineFromChar(char byte[],int mode);
/* 拆分short为两个char       */ void  splitToChar(short input,char *byte);
/* 二进制转十进制            */ short parseBinaryString(char buffer[],int start,int length);
/* 从文件读取程序            */ void  readCodeToMemory(CPU* cpu);
/* 由IP从内存初始化IR        */ void  readMemorytoIR(CPU* cpu);
/* 获取寄存器指向的真实数据  */ short realGetREG(CPU* cpu,int reg);
/* 设置寄存器指向的真实数据  */ void  realSetREG(CPU* cpu,int reg,short value);
/* 输入数据到指定的寄存器    */ void  inputToREG(CPU* cpu,int reg);
/* 输出指定寄存器里的数据    */ void  outputFromREG(CPU* cpu,int reg);
/* 调试输出寄存器状态        */ void  dumpStatus(CPU* cpu);
/* 最后输出内存代码段和数据段*/ void  dumpMemory(CPU* cpu);
/* 进行加减乘除与或运算      */ short doCalcuate(short a,short b,int mode);
/* 比较工具函数              */ short compare(short a,short b);
/* 执行跳转指令              */ short doGoto(CPU* cpu,char mode,short to);
/* 主循环函数                */ void  loop(CPU* cpu);
/* 指令执行，若返回1，则IP+=4*/ int   action(CPU* cpu);

/* 合并n个char为int或short，这里mode为COMBINE_INT或者COMBINE_CHAR */
int combineFromChar(char byte[],int mode){
    int res = 0;
    for (int i = 0; i < mode; i++) {
        res = (res<<8) | (byte[i] & 0xff); 
    }
    return res;
}
/* 拆分short为两个char */
void splitToChar(short input,char *byte){
    byte[1] = input & 0xff;
    byte[0] = ( input >> 8 ) & 0xff;
}
/* 二进制转十进制 */
short parseBinaryString(char buffer[],int start,int length){
    short tmp = 0;
    for(int j=length-1;j>=0;j--){ //对每个8位循环
        tmp+=(buffer[start+j]=='1') * pow(2,length-1-j);  // '1'->1 '0'->0
    }
    return tmp;
}
/* 从文件读取程序 */
void readCodeToMemory(CPU* cpu){
    FILE* file = fopen(INPUT_NAME,"r");
	if(file==NULL){
	    printf("open file error\n");
    }else{
        char buffer[LINE_LENGTH+1];
        for(int memPos = 0;fgets(buffer,LINE_LENGTH,file)!=NULL;memPos+=4){
            cpu->MEM[memPos]   = (char)parseBinaryString(buffer,0,8);//指令数
            cpu->MEM[memPos+1] = (char)parseBinaryString(buffer,8,8);//操作数
            char val[2];
            splitToChar(parseBinaryString(buffer,16,16),val);
            cpu->MEM[memPos+2] = val[0];
            cpu->MEM[memPos+3] = val[1];//立即数
        }
	}
	fclose(file);
}
/* 由IP从内存初始化IR */
void readMemorytoIR(CPU* cpu){
    cpu->IR[0] = cpu->MEM[cpu->IP];  //指令位
    cpu->IR[1] = cpu->MEM[cpu->IP+1];//对象位
    char nrVal[2];
    nrVal[0] = cpu->MEM[cpu->IP+2];
    nrVal[1] = cpu->MEM[cpu->IP+3];
    cpu->NR = (short)combineFromChar(nrVal,COMBINE_SHORT); //立即数
}
/* 获取寄存器编号指向的真实数据 */
short realGetREG(CPU* cpu,int reg){
    if(reg==0){ //为了统一读取过程，规定0为立即数
        return cpu->NR;
    }
    int regValue = cpu->REG[reg-1];
    if(reg>=5){  //对地址寄存器进行读取内存操作，此时regValue为内存地址
        char ramValue[2];
        ramValue[0] = cpu->MEM[regValue];
        ramValue[1] = cpu->MEM[regValue+1];
        return (short)combineFromChar(ramValue,COMBINE_SHORT);
    }else{
        return regValue;
    }
}
/* 设置寄存器编号指向的真实数据 */
void realSetREG(CPU* cpu,int reg,short value){
    if(reg>=5){ //对地址寄存器进行写入内存操作，此时regValue为内存地址
        int regValue = cpu->REG[reg-1];
        char ramValue[2];
        splitToChar(value,ramValue);
        cpu->MEM[regValue] = ramValue[0];
        cpu->MEM[regValue+1] = ramValue[1];
    }else{
        cpu->REG[reg-1] = value;
    }
}
/* 输入数据到指定的寄存器 */
void inputToREG(CPU* cpu,int reg){
    short input;
    printf("in:\n");
    scanf("%hd",&input);
    realSetREG(cpu,reg,input);
}
/* 输出指定寄存器里的数据 */
void outputFromREG(CPU* cpu,int reg){
    printf("out: %hd\n",realGetREG(cpu,reg));
}
/* 调试输出寄存器状态 */
void dumpStatus(CPU* cpu){
    printf("ip = %hd\nflag = %hd\nir = %hd\n",cpu->IP,cpu->FLAG,(short)combineFromChar(cpu->IR,COMBINE_SHORT));
    printf("ax1 = %hd ax2 = %hd ax3 = %hd ax4 = %hd\n",cpu->REG[0],cpu->REG[1],cpu->REG[2],cpu->REG[3]);
    printf("ax5 = %hd ax6 = %hd ax7 = %hd ax8 = %hd\n",cpu->REG[4],cpu->REG[5],cpu->REG[6],cpu->REG[7]);
}
/* 调试输出内存代码段和数据段 */
void dumpMemory(CPU* cpu){
    printf("\ncodeSegment :\n");
    int pos=0;
    for(int i=0;i<OUT_LINES;i++){
        for(int j=0;j<CODE_COUNT;j++){
            char current[CODE_GROUP];
            for(int k=0;k<CODE_GROUP;k++){
                current[k] = cpu->MEM[pos+k];
            }
            printf(j>0?" %d":"%d",combineFromChar(current,COMBINE_INT));
            pos+=CODE_GROUP;
        }
        printf("\n");
    }
    pos=16384;
    printf("\ndataSegment :\n");
    for(int i=0;i<OUT_LINES;i++){
        for(int j=0;j<DATA_COUNT;j++){
            char current[DATA_GROUP];
            for(int k=0;k<DATA_GROUP;k++){
                current[k] = cpu->MEM[pos+k];
            }
            printf(j>0?" %hd":"%hd",combineFromChar(current,COMBINE_SHORT));
            pos+=DATA_GROUP;
        }
        printf("\n");
    }
}
/* 进行加减乘除与或运算 */
short doCalcuate(short a,short b,int mode){
    switch (mode){
        case CMD_ADD:
            return a+b;
        case CMD_SUB:
            return a-b;
        case CMD_MUL:
            return a*b;
        case CMD_DIV:
            return a/b;
        case CMD_AND:
            return a&&b;
        case CMD_OR:
            return a||b;
        default:
            break; //除非代码错了。
    }
}
/* 比较工具函数 */
short compare(short a,short b){
    return a>b?1:(a<b?-1:0);
}
/* 执行跳转指令 */
short doGoto(CPU* cpu,char mode,short to){
    int flag = 0;
    switch (mode){
        case JMP_POS:
            flag = cpu->FLAG==1;
            break;
        case JMP_NEG:
            flag = cpu->FLAG==-1;
            break;
        case JMP_ZERO:
            flag = cpu->FLAG==0;
            break;
        case JMP_ALL:
            flag = 1;
            break;
        default:
            break;//什么都不会发生。flag已经是0了。
    }
    if(flag){
        cpu->IP += to;
        return 1;
    }
    return 0;
}
/* 指令执行，若返回1，则IP+=4*/
int action(CPU* cpu){
    short ctl  = ((short)cpu->IR[1]) & 0xff ;//防止符号位影响操作数值
    short ctl1 = ctl/OBJ_LEN;
    short ctl2 = ctl%OBJ_LEN;
    switch(cpu->IR[0]){
        case CMD_IN:
            inputToREG(cpu,ctl1);
            break;
        case CMD_OUT:
            outputFromREG(cpu,ctl1);
            break;
        case CMD_COPY:
            if(ctl2==0&&ctl1>=4){
                cpu->REG[ctl1-1] = cpu->NR; //寄存器>=5，另一个是立即数，这是内存分配
            }else{
                realSetREG(cpu,ctl1,realGetREG(cpu,ctl2));
            }
            break;
        case CMD_ADD:
        case CMD_SUB:
        case CMD_MUL:
        case CMD_DIV:
        case CMD_AND:
        case CMD_OR:
            realSetREG(cpu,ctl1,
                doCalcuate(realGetREG(cpu,ctl1),realGetREG(cpu,ctl2),cpu->IR[0])
            );
            break;
        case CMD_NOT:{
            short notCtl = ctl1==0?ctl2:ctl1;
            realSetREG(cpu,notCtl,!realGetREG(cpu,notCtl));
            break;
        }
        case CMD_CMP:
            cpu->FLAG = compare(realGetREG(cpu,ctl1),realGetREG(cpu,ctl2));
            break;
        case CMD_GOTO:
            if(doGoto(cpu,cpu->IR[1],cpu->NR)){
                return 0;
            }
            break;
    }
    return 1;
}
/* 主循环函数 */
void loop(CPU* cpu){
    readMemorytoIR(cpu);
    while(cpu->IR[0]!=CMD_STOP){
        if(action(cpu)){
            cpu->IP+=4;
        }
        dumpStatus(cpu);
        readMemorytoIR(cpu);
    }
    cpu->IP+=4;//停机也需要+=4 别问我为什么 示例是这么写的
}
/* 入口 */
int main(void){
    CPU cpuInstance;
    CPU* cpu = &cpuInstance;
    readCodeToMemory(cpu);
    loop(cpu);
    dumpStatus(cpu);
    dumpMemory(cpu);
    system("p");
    return 0;
}
