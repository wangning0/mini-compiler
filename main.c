#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

int token;
int token_val; // value of current token
char *src, *old_src;
int poolsize;
int line;

int* text; // 代码段
int* old_text; // 旧的代码段
int* stack; // stack
char* data; // 数据段

int *current_id; // 当前的parse id
int *symbols;  // 标识表

/*
	token 标识符返回的标记 理论上所有的变量返回的标记都应该是 Id，但实际上由于我们还将在符号表中加入关键字如 if,while 等，它们都有对应的标记
	hash 标识符的哈希值
	name 存放标识符本身的字符串
	class 该标识符的类别，如数字，全局变量或局部变量等
	type 标识符的类型
	value 存放这个标识符的值
	BXXX C语言中标识符可以是全局的也可以是局部的，当局部标识符与全局标识符相同时，用作保存全部标识符的信息
*/

enum {
	Token, Hash, Name, Type, Class, Value, BType, BClass, BValue,IdSize
};

/*
	寄存器
	pc 程序计数器，存放的是内存地址，改地址中存放着下一条要执行的计算机指令
	sp 指针寄存器，永远指向当前的栈顶。栈是从高地址向低地址增长的，所以入栈时SP的值减少
	bp 基址指针，在调用函数时会使用它
	ax 通用寄存器，我们的虚拟机中，它用于存放一个指令执行后的结果
*/
int *pc, *bp, *sp, ax, cycle;

// 指令集, 顺序的安排时有意的 表示从0开始
enum { LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
	   OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
	   OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};

// 支持的标记，顺序与它们在C语言中的优先级有关,注意的是，有些字符，它们自己就构成了标记，如右方括号]或波浪号~等
enum {
	Num = 128, Fun, Sys, Glo, Loc, Id, Char, Else, Enum, If, Int, Return, Sizeof, While,
	Assign, Cond, Lor, Lan, Or, Xar, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul,
	Div, Mod, Inc, Dec, Brak
};

void next() { 	// 用于词法分析，获取下一个标记
	char *last_pos;
	int hash;
	
	/*
		while 的原因是，跳过这些我们不识别的字符，同时还用它来处理空白字符
	*/
	while(token = *src) {
		+=src;
		if(token == '\n') {
			++line;
		} else if(token == '#') {
			// 宏定义 不支持，直接跳过
			while(*src != 0 && *src != '\n') {
				src++;
			}
		} else if((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z')) {
			last_pos = src - 1;
			hash = token;
			
			while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z')) {
				hash = hash * 147 + *src; // hash function http://www.cse.yorku.ca/~oz/hash.html
				src++
			}
			
			current_id = symbols;
			while(current_id[Token]) {
				if(current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
					token = current_id[Token];
					return;
				}
				current_id = current_id + IdSize;
			}
			
			current_id[Name] = (int)last_pos;
			current_id[Hash] = hash;
			token = current_id[Token] = Id;
			return;
		} 
	}
	
	return;
}

void expression(int level) {  // 用于解析一个表达式
	// do nothing
}

// 语法分析的入口
void program() {
	next();
	while(token > 0) {
		printf("token is: %c\n", token);
		next();
	}
}

int eval() {  // 虚拟机的入口
	/*
		MOV
		所有指令中最基础的一个，它用于将数据放进寄存器或内存地址中，x86的MOV指令有两个参数，分别是源地址和目标地址，MOV dest, source
		我们的虚拟机只有一个寄存器，而且识别这些参数的类型(数据还是地址)比较复杂，因此MOV被拆分成5个
		IMM <num> 将<num>放入寄存器ax中
		LC 将对应地址中的字符载入ax中，要求ax中存放地址
		LI 将对应地址中的整数载入ax中，要求ax中存放地址
		SC 将ax中的数据作为字符存放地址中， 要求栈顶存放地址
		SI 将ax中的数据作为整数存放入地址中，要求栈顶存放地址
		
		LC/SC 和 LI/SI 就是对应字符型和整型的存取操作
	*/
	/*
		PUSH
		作用是将ax的值放入栈中，这样做的原因时为了简化虚拟机的实现，我们只有一个寄存器ax
	*/
	/*
		JMP
		JMP <addr>是跳转指令，无条件的将当前的PC寄存器设置为指定的 <addr>
	*/
	/*
		JZ/JNZ
		为了实现if语句，我们需要条件判断相关的指令，这里我们只实现两个最简单的条件判断，即结果ax为零或不为零情况下的跳转
	*/
	/*
		子函数调用
		要引入的命令有CALL、ENT、ADJ、LEV
		CALL <addr> 与RET指令，CALL的作用是跳转到地址为<addr>的子函数，REt则用于从子函数中返回
		?? 为什么不直接用JMP指令??
		因为从子函数返回时，程序需要回到跳转前的地方继续运行，需要事先将这个位置信息存储起来，反过来，子函数要返回时，就需要获取并恢复这个信息。所以实际上我们把PC保存在栈中
	*/
	/*
		在实际调用函数时，不仅要考虑函数的地址，还要考虑如何传递参数和如何返回结果。
		这里我们约定，如果子函数有返回结果，那么就在返回时保存在ax中，它可以是一个值，
		可以是一个地址
	*/
	/*
		参数的传递
		
		因为我们的指令过于简单(如只能操作ax寄存器)，所以我们采用增加指令集的方式
		
		ENT 
		
		ENT <size>指的是enter，用于实现'make new call frame'的功能，即保存当前的栈指针，同时在栈上保留一定的空间，用来存放局部变量
		
		ADJ
		
		ADJ <size> 用于实现'remove arguments from frame' 在将调用子函数时压入栈中的数据清楚
		
		LEV
		
		因为我们指令集中没有POP指令
		
		LEA
		
		LEA <offset>获取参数
	*/
	int op, *tmp;
	while(1) {
		if(op == IMM) {
			ax = *pc++;
		} 
		else if(op == LC) {
			ax = *(char *)ax;
		} 
		else if(op == LI) {
			ax = *(int *)ax;
		} 
		else if(op == SC) {
			ax = *(char *)*sp++ = ax; // *sp++ 相当于退栈操作
		} 
		else if(op == SI) {
			*(int *)*sp++ = ax;
		} 
		else if(op == PUSH) {
			*--sp = ax;
		} 
		else if(op == JMP) {
			pc = (int *)*pc;  // 因为pc寄存器指向的是下一条指令，所以它存放的是JMP的参数
		} 
		else if(op == JZ) {
			pc = ax ? pc + 1 : (int *)*pc;
		} 
		else if(op == JNZ) {
			pc = ax ? (int *)*pc : pc + 1;
		} 
		else if(op == CALL) {
			*--sp = (int)(pc + 1);
			pc = (int *)*pc;
		} 
		else if(op == ENT) {
			*--sp = (int)bp;
			bp = sp;
			sp = sp - *pc++;
		} 
		else if(op == ADJ){
			sp = sp + *pc++;
		} 
		else if(op == LEV) {
			sp = bp;
			bp = (int *)*sp++;
			pc = (int *)*sp++;
		} 
		else if(op == LEA) {
			ax = (int)(bp + *pc++);
		} 
		else if (op == OR)  ax = *sp++ | ax;
		else if (op == XOR) ax = *sp++ ^ ax;
		else if (op == AND) ax = *sp++ & ax;
		else if (op == EQ)  ax = *sp++ == ax;
		else if (op == NE)  ax = *sp++ != ax;
		else if (op == LT)  ax = *sp++ < ax;
		else if (op == LE)  ax = *sp++ <= ax;
		else if (op == GT)  ax = *sp++ >  ax;
		else if (op == GE)  ax = *sp++ >= ax;
		else if (op == SHL) ax = *sp++ << ax;
		else if (op == SHR) ax = *sp++ >> ax;
		else if (op == ADD) ax = *sp++ + ax;
		else if (op == SUB) ax = *sp++ - ax;
		else if (op == MUL) ax = *sp++ * ax;
		else if (op == DIV) ax = *sp++ / ax;
		else if (op == MOD) ax = *sp++ % ax;
		else if (op == EXIT) { printf("exit(%d)", *sp); return *sp;}
		else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
		else if (op == CLOS) { ax = close(*sp);}
		else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
		else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
		else if (op == MALC) { ax = (int)malloc(*sp);}
		else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
		else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
		else {
			printf("unknown instruction:%d\n", op);
			return -1;
		}
	}
	return 0;
}

int main(int argc, char **argv) {
	int i, fd;
	
	argc--;
	argv++;
	
	poolsize = 256 * 1024;
	line = 1;
	
	if((fd = open(*argv, O_RDONLY)) < 0) {
		printf("不能打开文件");
		return -1;
	}
	
	if(!(src = old_src = malloc(poolsize))) {
		printf("分配不了这么大的内存");
	}
	
	if((i = read(fd, src, poolsize - 1)) <= 0) {
		printf("read() returned %d\n", i);
		return -1;
	}
	
	src[i] = 0;
	close(fd);
	
	if(!(text = old_text = malloc(poolsize))) {
		printf("分配不了这么大的内存给text area");
		return -1;
	}
	
	if(!(data = malloc(poolsize))) {
		printf("分配不了这么大的内存给数据段");
		return -1;
	}
	
	if(!(stack = malloc(poolsize))) {
		printf("分配不了这么大的内存给stack");
		return -1;
	}
	
	
	memset(text, 0, poolsize);
	memset(data, 0, poolsize);
	memset(stack, 0, poolsize);
	
	// 初始化代码
	bp = sp = (int *)((int)(stack) + poolsize);
	program();
	
	return eval();
	
}