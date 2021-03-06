#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

union Data {
    unsigned i;
    unsigned char bytes[4];
} data;

#define LEN_OF_HW_REG 23
#define LEN_OF_SIMP_REG 16
#define LEN_OF_RAM 4096
#define LEN_48_BITS 12
#define BUFFER_RAM_LEN 8+1 //string is 8 +1
#define BUFFER_INST_LEN 12+1
#define OPCODE 11
#define FALSE 0
#define NotIninterruptMode 0
#define IninterruptMode 1
#define TRUE 1
#define irq1Enable 1

#define MON_DIM  256
#define diskstatus IORegister[17]
#define reserved1 IORegister[18]
#define reserved2 IORegister[19]
#define monitoraddr IORegister[20]
#define monitordata IORegister[21]
#define monitorcmd IORegister[22] 

// define globals
// LG
unsigned mon[MON_DIM][MON_DIM] = { 0 };
unsigned char monitor_mat_bye[MON_DIM][MON_DIM] = { 0 };
int countcyc = 0;
int readcount, writecount = 0;

#define add 0
#define sub 1
#define mac 2
#define and 3
#define or 4
#define xor 5
#define sll 6
#define sra 7
#define srl 8
#define beq 9
#define bne 10
#define blt 11
#define bgt 12
#define ble 13   
#define bge 14
#define jal 15   
#define lw 16
#define sw 17
#define reti 18 
#define in 19
#define out 20
#define halt 21

#define irq0enable IORegister[0]
#define irq1enable IORegister[1]
#define irq2enable IORegister[2]
#define irq0status IORegister[3]
#define irq1status IORegister[4]
#define irq2status  IORegister[5]
#define irqhandler IORegister[6]
#define irqreturn IORegister[7]
#define leds IORegister[9]
#define timerenable IORegister[11]
#define timercurrent IORegister[12]
#define timermax IORegister[13]
#define diskcmd IORegister[14]
#define disksector IORegister[15]
#define diskbuffer IORegister[16]

int my_shift(num, shift) {
    int res = num >> shift;
    unsigned mask = (~(1 << 31));
    for (int i = 0; i < shift; i++)
    {
        res &= mask;
        mask = mask >> 1;
    }
    return res;

}

unsigned int countSetBits(unsigned int n)
{
    unsigned int count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

unsigned createMask(unsigned a, unsigned b)
{
    unsigned r = 0;
    for (unsigned i = a; i <= b; i++)
        r |= 1 << i;

    return r;
}

void remove_all_chars(char* str, char c) {
    char* pr = str, * pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

unsigned init_list(unsigned hardware_registers[]) {
    unsigned i = 0;
    for (i = 0; i < LEN_OF_HW_REG; i++)
        hardware_registers[i] = 0;
    
    return 0;
}

unsigned print_list(unsigned*list) {
    unsigned i = 0;
    for (i = 0; i < LEN_OF_RAM; i++)
        printf("%d\n", list[i]);
   
    return 0;
 }

void myprintf(char* s) {
    for (int i = 0; i < LEN_48_BITS; i++)
        printf("%c", s[i]);
    
}

void get_imemin_inst_from_file(FILE * file_imemin, char imemin_inst[LEN_OF_RAM][LEN_48_BITS]) {
    char buffer_inst[BUFFER_INST_LEN] = { 0 };
    char buffer_inst_inst_real_len[LEN_48_BITS] = { 0 };
    int i = 0;

    while (fgets(buffer_inst, BUFFER_INST_LEN, file_imemin)){
        if (buffer_inst[0] == 10) // only god knows this line
            continue;
       
        strcpy(imemin_inst[i], buffer_inst);
        //printf("%s,%d \n", imemin_inst[i], i);
        //myprintf(imemin_inst[i]);// debage line
        i++;
    }
}

void get_dmemin_ins_from_file(FILE* file_dmemin, unsigned dmemin_list_RAM[LEN_OF_RAM])
{
    char buffer[BUFFER_RAM_LEN] = { 0 };
    int ret;
    int i = 0;
    while (fgets(buffer, BUFFER_RAM_LEN, file_dmemin))
    {
        if (buffer[0] == 10) // only god knows this line
            continue;
        ret = strtol(buffer, NULL, 16);
        dmemin_list_RAM[i] = ret;
        i++;
       // printf("adress :%d value: %d \n",i ,dmemin_list_RAM[i-1]); //debage line
        
    }
}

void zero_holder(char* list)
{
    for (int i = 0; i < 4; i++)
        list[i] = 0;
}

void clear_interrupt(unsigned* IORegister) {
    for (int i = 3; i < 6; i++)
        IORegister[i] = 0;
}

void monitor(unsigned mon[MON_DIM][MON_DIM], unsigned target_pix, unsigned data, int* R, unsigned* IORegister)
{
    union Data data_to_yuv;
    data_to_yuv.i = data;
    int row = 0;
    int col = 0;
    //printf("the target pix: %d, data is :%d", target_pix, data);
    /*while (target_pix > MON_DIM)
    {
      
        target_pix -= MON_DIM;
        row++;
        printf("the a1:%d , the target pix:%d , thr row is=%d\n", R[5], target_pix, row);
    }*/
    //printf("the a1:%d , the target pix:%d , thr row is=%d\n", R[5], target_pix, row);
    row = target_pix / 256;
    col = (target_pix % 256);
   // printf("the a1:%d ,the target pix:%d ,col=%d, thr row is=%d,monitorcmd=%d\n",R[5], target_pix,col, row,monitorcmd);
    //col = target_pix;
    //printf("the index row:%d col=%d", row, col);

    mon[row][col] = data;
   
    monitor_mat_bye[row][col] = data_to_yuv.bytes[0];
    //printf("\nmonitor[%d][%d] data is: %u \n", row, col, data);
  
}

void Read_write_Disk(int dmemin[4096], int disk_list[2048], unsigned *IORegister)
{

    if (diskcmd==1)
    {
        readcount++;
    }
    if(diskcmd==2)
    {
        writecount++;
    }

    int i = 0;
    if (diskcmd == 1)        //read sector reads 16 lines from disk to dmemin
    {
        for (i = 0; i < 16; i++)
        {
            dmemin[diskbuffer + i] = disk_list[disksector*16 + i];
        }
    }
    if(diskcmd == 2)                    //write sector read from dmemin and write to disk
    {
        for (i = 0; i < 16; i++)
        {
            disk_list[disksector*16 + i] = dmemin[diskbuffer + i];
        }
    }
}

void get_reg_name_by_table(int reg_number, char reg_name_by_table[15]) {
    switch (reg_number)
    {
    case 0:
        strcpy(reg_name_by_table, "irq0enable");
        break;
    case 1:
        strcpy(reg_name_by_table, "irq1enable");
        break;
    case 2:
        strcpy(reg_name_by_table, "irq2enable");
        break;
    case 3:
        strcpy(reg_name_by_table, "irq0status");
        break;
    case 4:
        strcpy(reg_name_by_table, "irq1status");
        break;
    case 5:
        strcpy(reg_name_by_table, "irq2status");
        break;
    case 6:
        strcpy(reg_name_by_table, "irqhandler");
        break;
    case 7:
        strcpy(reg_name_by_table, "irqreturn");
        break;
    case 8:
        strcpy(reg_name_by_table, "clks");
        break;
    case 9:
        strcpy(reg_name_by_table, "leds");
        break;
    case 10:
        strcpy(reg_name_by_table, "display7seg");
        break;
    case 11:
        strcpy(reg_name_by_table, "timerenable");
        break;
    case 12:
        strcpy(reg_name_by_table, "timercurrent");
        break;
    case 13:
        strcpy(reg_name_by_table, "timermax");
        break;
    case 14:
        strcpy(reg_name_by_table, "diskcmd");
        break;
    case 15:
        strcpy(reg_name_by_table, "disksector");
        break;
    case 16:
        strcpy(reg_name_by_table, "diskbuffer");
        break;
    case 17:
        strcpy(reg_name_by_table, "diskstatus");
        break;
    case 18:
        strcpy(reg_name_by_table, "reserved");
        break;
    case 19:
        strcpy(reg_name_by_table, "reserved");
        break;
    case 20:
        strcpy(reg_name_by_table, "monitoraddr");
        break;
    case 21:
        strcpy(reg_name_by_table, "monitordata");
        break;
    case 22:
        strcpy(reg_name_by_table, "monitorcmd");
        break;
    default:
        break;
    }
}

unsigned Execute(unsigned opcode, int *rd, int *rs, int*rt, int *rm, int *immediate1,
    int *immediate2, int *MEM, unsigned*pc,int *R , unsigned* IORegister, 
    int * interrupt_mode, FILE* leds_file, int *disk_list, FILE* Files[15],int cycles) {
    unsigned PC_holder = *pc;
    
    int irq = (irq0enable & irq0status) | (irq1enable & irq1status) | (irq2enable & irq2status);
    //printf("the irq0enable is:%d irq0status:%d , pc is: %d \n", irq0enable , irq0status,*pc);
    unsigned temp_leds;
    unsigned mask = createMask(0, 11);
    unsigned msb = 0;
    
    char reg_name_by_table[15] = { 0 };
   
    //printf("##### $t1=%d diskcmd=%d diskstatus=%d countcyc=%d $t0=%d $rs=%d $rt=%d $rm=%d imm1=%d imm2=%d readcount=%d writecount=%d\n", 
    //    R[8],diskcmd, diskstatus, countcyc,R[7],*rs,*rt,*rm,*immediate1,*immediate2,readcount,writecount);
    
    if (diskstatus)
    {
        countcyc++;
    }
    if (countcyc == 5){
        countcyc = 0;
        diskstatus = 0;
        irq1status = 1;

    }



    if ((irq == 1) && ((*interrupt_mode) == NotIninterruptMode))
    {
        irqreturn = (*pc);
        //IORegister[6] is irq handler
        (*pc) = irqhandler;
        (*interrupt_mode) = 1;
        return TRUE;
    }

    //printf("\nthe x=%d \n", R[4]);
    switch (opcode)
    {    
    case add:
        if ((*rd)!=0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = R[*rs] + R[*rt] + R[*rm];
        break;
    case sub:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = R[*rs] - R[*rt] - R[*rm];
        //printf(" R[rd]=%d R[rs]=%d,R[rt]=%d,R[rm]=%d \n", R[*rd], R[*rs], R[*rt], R[*rm]);
        break;
    case mac:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = (R[*rs] * R[*rt]) + R[*rm];
        break;
    case and:
          //printf(" R[rd]=%x R[rs]=%x,R[rt]=%x,R[rm]=%x \n", R[*rd], R[*rs], R[*rt], R[*rm]);
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = R[*rs] & R[*rt] & R[*rm];
         // printf(" R[rd]=%x R[rs]=%x,R[rt]=%x,R[rm]=%x \n", R[*rd], R[*rs], R[*rt], R[*rm]);
        break;
    case or:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = R[*rs] | R[*rt] | R[*rm];
        break;
    case xor:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = R[*rs] ^ R[*rt] ^ R[*rm];
        break;
    case sll:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = R[*rs] << R[*rt];
        break;
    case sra:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2)) 
                R[*rd] = (R[*rs] >> R[*rt]);
        break;
    case srl:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = my_shift(R[*rs],R[*rt]);
        //printf("R[rd]=%d\n", R[*rd]);
        break;
    case beq:
        if (R[*rs] == R[*rt]) 
            *pc = R[*rm] & mask;
        break;
    case bne: 
        if (R[*rs] != R[*rt]) 
            *pc = R[*rm] & mask;
        break;
    case blt:
        if (R[*rs] < R[*rt]) 
            *pc = R[*rm] & mask;
        break;
    case bgt:
        if (R[*rs] > R[*rt])
            (*pc) = (R[*rm]) & mask;
        break;
    case ble:
        if (R[*rs] <= R[*rt])
           *pc = R[*rm] & mask;
        break;
    case bge:
        if (R[*rs] >= R[*rt])
            *pc = R[*rm] & mask;
        break;
    case jal:
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
        {
            R[*rd] = (unsigned)((*pc) + 1);
            //printf(" jal %d", PC_holder);
            (*pc) = (unsigned)((R[*rm]) & mask);
        }
        break;
    case lw:
         //printf(" the adress is %d\n", (MEM[R[*rs] + R[*rt]]));
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = MEM[(R[*rs]) + R[*rt]] + R[*rm];
        break;
    case sw:
        MEM[R[*rs] + R[*rt]] = R[*rm] + R[*rd];
        break;
    case reti:
        //here will return from an interrupt.
        //need to back the original value of Pc
        //increment PC - to not looping on the same PC
        //note that the if at the end of the function not enter since the handler is diff.
        //finally back to not in interrupt mode -> interrupt_mode = 0.
        *pc = irqreturn;
        (*interrupt_mode) = 0;
        clear_interrupt(IORegister);
        break;
    case in:
        // here we read from IO registers
        //if user want to readd from monitor -> return 0.
        if ((R[*rs] + R[*rt]) == 22)
        {
            R[*rd] = 0;
            break;
        }
        if ((*rd) != 0 && ((*rd) != 1) && ((*rd) != 2))
            R[*rd] = IORegister[R[*rs] + R[*rt]];

        get_reg_name_by_table(R[*rs] + R[*rt], reg_name_by_table);
      //  fprintf(Files[8], "%X READ %s %.8X \n", cycles, reg_name_by_table, R[*rd]);
       // fflush(Files[8]);

        break;
    case out:
        // here we writing to IO registers
        // hold led status before possible change
        temp_leds = leds; 

        // handle with 7segdisplay
        if ((R[*rs] + R[*rt]) == 10 &&(IORegister[R[*rs] + R[*rt]] != R[*rm])) 
            fprintf(Files[11], "%d %.8X \n", cycles, R[*rm]);
        
       

        IORegister[R[*rs] + R[*rt]] = R[*rm];

        
        // handle with harddisk
        if ((diskcmd != 0) && (diskstatus == 0) && (countcyc==0)){
            
            Read_write_Disk(MEM, disk_list, IORegister);
            diskstatus = 1;
        }
        
        
        // handle led status change
        if (9 == R[*rs] + R[*rt] && leds != temp_leds ) {
            //FILE* leds_file = fopen("leds.txt", "a");
            fprintf(leds_file, "%d %x\n", timercurrent, leds);
            //fclose(leds_file);
        }
        
        if (monitorcmd) {
            
            monitor(mon, monitoraddr, monitordata,R, IORegister);
        }
        
        get_reg_name_by_table(R[*rs] + R[*rt], reg_name_by_table);
        fprintf(Files[8], "%X WRITE %s %.8X \n", cycles, reg_name_by_table, R[*rm]);
       
        break;
    case halt:
        //printf("\n############ halt ############\n");
        return FALSE;
    }

    //printf(" monitoraddr= %d, monitordata =%d , monitorcmd =%d, $t0=%d , $t1=%d , $a0=%d , $s0=%d,$s1=%d \n",monitoraddr,monitordata,monitorcmd,R[7],R[8],R[4],R[10],R[11]);
    if ((*pc) == PC_holder)
      (*pc)++;
    //printf(" opcode: %d , RD = %d, RS = %d ,RT = %d,RM = %d ,R[rd]=%d,", opcode, *rd, *rs, *rt, *rm, R[*rd]);
    
    return TRUE;
}

void time_handler(unsigned* IORegister) {
    if (timerenable) {
        if (timercurrent == timermax) {
            irq0status = 1;
            timercurrent = 0;
        }
        else
            timercurrent++;
    }
}

int func(int x) {
    int mask = 0x80000000; // 00000010053545000120202032
        while ((mask & x) != mask)
        {
            x = x | mask;
            mask >>= 1;
        }
    return x;
}

void mystrcpy(char* dest , char* source) {
    for (int i = 0; i < 12; i++)
        dest[i] = source[i];
}

void Creat_dmemout(int dmemin_list_RAM[LEN_OF_RAM],FILE* dmemout_file){
    
    for (int i = 0;  i < LEN_OF_RAM;  i++)
        fprintf(dmemout_file, "%.8X\n", dmemin_list_RAM[i]);
    fclose(dmemout_file);
}

void Creat_regout(int simp_reg[LEN_OF_SIMP_REG], FILE* regout_file)
{
    for (int i = 3; i < LEN_OF_SIMP_REG; i++)
        fprintf(regout_file, "%.8X\n", simp_reg[i]);
    fclose(regout_file);
}

void Creat_cycles(int cycles, FILE* regout_file) {
        fprintf(regout_file, "%d", cycles);
    fclose(regout_file);
}

void get_irq2in_list(FILE *file_irq2in, int irq2_list[LEN_OF_RAM])
{
    char buffer[BUFFER_RAM_LEN] = { 0 };
    int ret;
    int i = 0;
    while (fgets(buffer, BUFFER_RAM_LEN, file_irq2in))
    {
        ret = strtol(buffer, NULL, 10);
        irq2_list[i++] = ret;
        //printf(" the irq :%d \n", irq2_list[i-1]); //debage line
    }
    //print_list(irq2_list);
}

int Is_value_in_list(int x, int list[LEN_OF_RAM]) {
    for (int i = 0; i < LEN_OF_RAM; i++){
        if (x==list[i])
            return TRUE;
        return FALSE;
    }
}

void irq1_handler(int cycles, int irq2_list[LEN_OF_RAM], unsigned IORegister[LEN_OF_HW_REG]) {
    if (Is_value_in_list(cycles, irq2_list) && cycles != 0)
        irq1status = 1;
    else
        irq1status = 0;
}   

void open_files(FILE** Files ,char ** argv) {
    Files[0] = 0;
    for (int j = 1; j < 15; j++) {
        if (j>4)
        {
            if (j==14)
            {
                Files[j] = fopen(argv[j], "wb");
                if (Files[j] == NULL)
                    exit("Problem openning file");
            }
            else
            {
                Files[j] = fopen(argv[j], "w");
                if (Files[j] == NULL)
                    exit("Problem openning file");
                //printf("Success %s\n", argv[j]);
            }
        }
        else
        {
            Files[j] = fopen(argv[j], "r");
            if (Files[j] == NULL)
                exit("Problem openning file");
             //printf("Success %s\n", argv[j]);
        }
       
    }


}

void close_files(FILE** Files, char** argv)
{
    for (int j = 1; j < 15; j++) {
        //printf("GOT HERE %d \n", j);
        fclose(Files[j]);
        //printf("Closed %s\n", argv[file_num]);
    }
}

void decode(int simp_reg[LEN_OF_SIMP_REG],char current_inst[LEN_48_BITS], char holder[4],
    unsigned *OP_Code, int *register_RD, int *register_RS, int *register_RT, int *register_RM,
    unsigned *immediate1, unsigned* immediate2, int *immediate1_n, unsigned mask)
 {
    //get opcode
    holder[0] = current_inst[0];
    holder[1] = current_inst[1];
    *OP_Code = strtol(holder, NULL, 16);

    //printf(",OPCODE_PRE: %d ", *OP_Code);
    zero_holder(holder);

    //get RD
    holder[0] = current_inst[2];
    *register_RD = strtol(holder, NULL, 16);
    zero_holder(holder);

    //get RS
    holder[0] = current_inst[3];
    *register_RS = strtol(holder, NULL, 16);
    zero_holder(holder);

    //get RT
    holder[0] = current_inst[4];
    *register_RT = strtol(holder, NULL, 16);
    zero_holder(holder);

    //get RM
    holder[0] = current_inst[5];
    *register_RM = strtol(holder, NULL, 16);
    zero_holder(holder);

    //get imm1
    holder[0] = current_inst[6];
    holder[1] = current_inst[7];
    holder[2] = current_inst[8];
    *immediate1 = strtol(holder, NULL, 16);

    //immediate1 = func(immediate1);
    // if the MSB is bigger than 7 in hexa then is
    //negative number, need to set the bits to 1.
    if (holder[0] > '7')
        (*immediate1) |= mask;
    //printf(",the imm1 : %d \n", (*immediate1));
    simp_reg[1] = *immediate1;
    zero_holder(holder);

    //get imm2
    holder[0] = current_inst[9];
    holder[1] = current_inst[10];
    holder[2] = current_inst[11];
    *immediate2 = strtol(holder, NULL, 16);
    // if the MSB is bigger than 7 in 
    //hexa then is negative number
    //need to set the bits to 1.        
   
    if(holder[0] > '7')
        (*immediate2)|= mask;
    simp_reg[2] = *immediate2;
    zero_holder(holder);

}

void HarDdisk_to_list(FILE * diskinfile,int disk_list[2048]) {
    char buffer[BUFFER_RAM_LEN] = { 0 };
    int ret = 0;
    int i = 0;
    while (fgets(buffer, BUFFER_RAM_LEN, diskinfile))
    {
        ret = strtol(buffer, NULL, 16);
        //printf(" the %d", ret);
        disk_list[i++] = ret;
        //printf("%x\n", disk_list[i-1]); //debage line
    }
}

void creatmonitor(FILE* motinor_file)
{// here we creat monitor.txt
    for (int i = 0;i <= MON_DIM; i++)
    {
        for (int j = 0; j <= MON_DIM; j++)
        {
            fprintf(motinor_file,"%.2x\n", mon[i][j]);
        }
    }
}

void creatmonitoryuv(FILE* motinoryuv_file)
{   // here we creat monitor.yuv
    //here we writting to binary file
   /*
    for (int i = 0;i <= MON_DIM; i++)
    {
        for (int j = 0; j <= MON_DIM; j++)
        { 
           // printf("%x", mon[i][j]);
            //fprintf(motinoryuv_file, "%.2x", mon[i][j]);
            fwrite(&(monin1bye[i][j]), sizeof(unsigned char), 1, motinoryuv_file);
            printf("i=%d,j=%d, data = %d\n",i,j, monin1bye[i][j]);
            fflush(motinoryuv_file);
        }
    }*/

    fwrite(monitor_mat_bye, sizeof(monitor_mat_bye), 1, motinoryuv_file);



}   

void creatdiskout(int disk_list[2048], FILE* diskout_file) {
    for (int i = 0; i < 2048; i++)
        fprintf(diskout_file, "%.8X\n", disk_list[i]);
}


int main(int argc, char** argv)
{

    FILE* Files[15] = { 0 };
    open_files(Files, argv);
    
    //update instruction to imemin_inst matrix
    //max number of irq is 4096 == number of RAM
    int irq2_list[LEN_OF_RAM] = { 0 };
    get_irq2in_list(Files[4], irq2_list);

    //1 - not in interapt mode
    //0 - in interrupt mode
    int interrupt_mode = 0;

    //SIMP has 16 registers from 0-15, when 0,1,2 unchangeable
    int simp_reg[LEN_OF_SIMP_REG] = { 0 };
    simp_reg[0] = 0; /*Init the registers*/

    unsigned IORegister[LEN_OF_HW_REG] = { 0 };
    init_list(IORegister);

    // init 48 bits for each instruction In imemin.txt
    //update instruction to imemin_inst matrix
    char imemin_inst[LEN_OF_RAM][LEN_48_BITS] = { 0 };
    get_imemin_inst_from_file(Files[1], imemin_inst);

    //read from dmemin
    // init 32 bits for each instruction In dmemin.txt
    int dmemin_list_RAM[LEN_OF_RAM] = { 0 };
    get_dmemin_ins_from_file(Files[2], dmemin_list_RAM);


    //get list from harDdisk
    int disk_list[2048] = { 0 };
    HarDdisk_to_list(Files[3], disk_list);


    //PC register need to cange to 12bits*/
    unsigned program_counter = 0;

    //fetch
    char current_inst[LEN_48_BITS] = { 0 };
    char current_inst_trace[13] = { 0 };
    char holder[4] = { 0 };

    // init registers before decode
    //init mask to handle with negativ value of imm
    //cycles count the cycles number of CPU
    unsigned OP_Code = 0;
    int register_RD = 0;
    int register_RS = 0;
    int register_RT = 0;
    int register_RM = 0;
    unsigned immediate1 = 0;
    unsigned immediate2 = 0;
    int immediate1_n = 0;
    unsigned mask = 0xFFFFF000;
    unsigned running = 1;
    int cycles = 0;
    
    while (running)
    {
        irq1_handler(cycles, irq2_list, IORegister);
        time_handler(IORegister);

        // here we fetch the currnt instraction
        mystrcpy(current_inst, imemin_inst[program_counter]);
        mystrcpy(current_inst_trace, imemin_inst[program_counter]);
        current_inst_trace[12] = '\0';
        //-------------------------------//
      //  printf("\n");
      //  myprintf(current_inst);
      //  printf(" programcounter=%d\n", program_counter);
        //-------------------------------//
        
        //here we decode the instruction
        decode(simp_reg, current_inst, holder, &OP_Code, &register_RD, &register_RS,
            &register_RT, &register_RM, &immediate1, &immediate2, &immediate1_n, mask);
        
        
        //-------------------------------//
        //here we writing to trace.txt
        //this is trace on simp registers
        fprintf(Files[7], "%.3X %s %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X\n",
            program_counter, current_inst_trace,
            simp_reg[0], simp_reg[1], simp_reg[2], simp_reg[3], simp_reg[4], simp_reg[5], simp_reg[6], simp_reg[7],
            simp_reg[8], simp_reg[9], simp_reg[10], simp_reg[11], simp_reg[12], simp_reg[13], simp_reg[14], simp_reg[15]);
        
        //-------------------------------//
        //here we excetute the instraction
        running = Execute(OP_Code, &register_RD, &register_RS, &register_RT, &register_RM
            ,&immediate1, &immediate2, dmemin_list_RAM, &program_counter, 
            simp_reg, IORegister, &interrupt_mode,Files[10], disk_list,Files, cycles);

        //count cycles time
        cycles++;
    }
        
    //-------------------------------//
   // printf("$v0: %d $t0: %d ,$t1: %d ,ram is:%d ,pc: %d ", simp_reg[3],simp_reg[7], simp_reg[8], dmemin_list_RAM[33], program_counter);
    //-------------------------------//

    Creat_dmemout(dmemin_list_RAM, Files[5]);
    Creat_regout(simp_reg, Files[6]);
    Creat_cycles(cycles, Files[9]);
    creatmonitor(Files[13]);
    creatmonitoryuv(Files[14]);
    creatdiskout(disk_list, Files[12]);
    close_files(Files, argv);

    return 0;
}