/*
Chip-8 Emulator
Author: James Anthony Arcenas

Specs for this program:

All variables must have a declared value.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/***********************************************
 CHIP-8 SPECIFICATIONS
 ***********************************************/
#define MEMORY 4096
#define REGISTERS 16
#define STACK_SIZE 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define KEYPAD_SIZE 16


/*struct for the chip-8*/
typedef struct {
    uint16_t memory[MEMORY]; // 4K memory
    uint8_t V[REGISTERS];   // 16 registers (V0 to VF)
    uint16_t I;             // Index register
    uint16_t pc;            // Program counter
    uint16_t stack[STACK_SIZE]; // Stack for subroutine calls
    uint8_t sp;             // Stack pointer
    uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT]; // Display buffer
    uint8_t keypad[KEYPAD_SIZE]; // Keypad state
    uint8_t delay_timer;
    uint8_t sound_timer;
    
} Chip8;


/*protoypes*/
int boot(Chip8 *chip8);
int rom_loader(Chip8 *chip8, const char *rom_path);


int main(){
    Chip8 chip8;
    boot(&chip8);

    char *rom_path = "roms/breakout"; // Path to the ROM file
    rom_loader(&chip8, rom_path);
    return 0;
}

int boot(Chip8 *chip8){
    
    memset(chip8,0,sizeof(Chip8)); // Clear all memory and registers
    chip8->pc = 0x200; // Set program counter to start of program
    srand((uint16_t) time(NULL));
    return 0;
}

int rom_loader(Chip8 *chip8, const char *rom_path){
    FILE *rom_file = fopen(rom_path, "rb"); //file pointer to the rom file
    if (!rom_file) {
        fprintf(stderr, "Failed to open ROM file: %s\n", rom_path);
        return -1;
    }

    /**********This block of code gets the size of the ROM file***********/
    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    rewind(rom_file);
    
    /*Memory Validation*/
    if(rom_size > (MEMORY - 0x200)) {
        fprintf(stderr, "ROM file is too large to fit in memory: %s\n", rom_path);
        fclose(rom_file);
        return -1;
    }
    /*load the rom at address 0x200*/
    size_t bytes_read = fread(&chip8->memory[0x200], 1, rom_size , rom_file);
    if (bytes_read == 0) {
        fprintf(stderr, "Failed to read ROM file: %s\n", rom_path);
        fclose(rom_file);
        return -1;
    }

    fclose(rom_file);
    return 0;

}

int fde_loop(Chip8 *chip8){
    /*fetch*/
    uint16_t opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1]; // Fetch the opcode
    chip8->pc += 2; // Increment the program counter
    /*
    X is second nibble bits 8-11. used to look one of the 16 registers
    Y is third nibble bits 4-7. used to look one of the 16 registers
    N is fourth nibble bits 0-3. 4 bit number
    NN is the last two nibbles bits 0-7. 8 bit number
    NNN is the last three nibbles bits 0-11. 12 bit immediate memory address
    
    */
    uint16_t first_nibble = (opcode & 0xF000) >> 12; // Extract the first nibble from the opcode
    uint8_t x = (opcode & 0x0F00) >> 8; // Extract the X value from the opcode
    uint8_t y = (opcode & 0x00F0) >> 4; // Extract the Y value from the opcode
    uint8_t n = (opcode & 0x000F); // Extract the N value from the opcode
    uint8_t nn = (opcode & 0x00FF); // Extract the NN value from the opcode 
    uint16_t nnn = (opcode & 0x0FFF); // Extract the NNN value from the opcode 


    // Decode and execute the opcode
    switch(first_nibble) {
        case 0x1:
            chip8->pc = nnn; // Jump to address NNN
            break;
        case 0x2: //subroutine call
                chip8->stack[chip8->sp] = chip8->pc; // Save the return address
                chip8->sp++; //increment stackpointer by 1
                chip8->pc =nnn;
                break;
        case 0x6:
            chip8->V[x] = nn; // Set Vx to NN
            break;
        case 0x7:
            chip8->V[x] += nn; // Add NN to Vx
            break;
        case 0xA:
            chip8->I = nnn; // Set I to NNN
            break;
        case 0xB: // jump to NNN
            chip8->pc = chip8->V[0]+nnn;
            break;
        case 0x3:
            if(chip8->V[x] == nn){
                chip8->pc += 2;
            }
            break;
        case 0x4:
            if(chip8->V[x] != nn){
                chip8->pc += 2;
            }
            break;
        case 0x5:
            if(chip8->V[x] == chip8->V[y]){
                chip8->pc +=2;
            }
            break;
        // system instructions
        case 0x0:
            switch(nn){ 
                case 0xE0:
                    memset(chip8-> display, 0, sizeof(chip8->display)); // Clear the display
                    break;
                case 0xEE:
                    chip8->pc = chip8->stack[--chip8->sp]; // Return from subroutine
                    break;
                default:
                    fprintf(stderr, "Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;

        //ALU
        case 0x8:
            switch (n){
                case 0x0:
                    chip8 -> V[x] = chip8 -> V[y];
                    break;
                case 0x1:
                    chip8 -> V[x] = chip8 -> V[x] | chip8 -> V[y]; //Bitwise OR
                    break;
                case 0x2:
                    chip8 -> V[x] = chip8 -> V[x] & chip8 -> V[y]; //Bitwise AND
                    break;
                case 0x3:
                    chip8 -> V[x] = chip8 -> V[x] ^ chip8 -> V[y]; //Bitwise XOR
                    break;
                case 0x4:{ // Addition
                    uint8_t flag = 0;
                    if(chip8->V[x] + chip8->V[y] > 0xFF){
                        flag += 1;
                    }
                    else{
                        flag = 0;
                    }
                    chip8->V[x] += chip8->V[y];
                    chip8->V[0xF] = flag;
                    break;}
                case 0x5:{ //Subtraction
                    uint8_t flag = 0;
                    if(chip8->V[x] >= chip8->V[y]){
                        flag+=1;
                    }
                    else{
                        flag = 0;
                    }
                    chip8->V[x] -= chip8->V[y];
                    chip8->V[0xF] = flag;
                    break;}
                case 0x7:{ // Inverse Subtraction
                    uint8_t flag = 0;
                    if(chip8->V[y] >= chip8->V[x]){
                        flag+=1;
                    }
                    else{
                        flag = 0;
                    }
                    chip8->V[x] = chip8->V[y] - chip8->V[x];
                    chip8->V[0xF] = flag;
                    break;}

                /*Bit Shifts*/
                case 0x6:{ //Divide by 2
                    uint8_t flag=0;
                    if((chip8->V[x] & 0x1) == 1){
                        flag +=1;
                    }
                    else{
                        flag +=0;
                    }
                    chip8->V[x] >>= 1; //LSB
                    chip8->V[0xF] = flag;
                    break;}
                case 0xE:{ //Multiply by 2
                    uint8_t flag=0;
                    if((chip8->V[x] & 0x80) == 0x80){
                        flag +=1;
                    }
                    else{
                        flag +=0;
                    }
                    chip8->V[x] <<= 1; //MSB
                    chip8->V[0xF] = flag;
                    break;}
                default:
                    fprintf(stderr, "Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
        case 0xC:{
            chip8->V[x] = (rand() % 256) & nn;
            break;
        }
        
        //Keyboard
        case 0xE:
            switch(nn){
                case 0x9E:
                    if(chip8->keypad[chip8->V[x]] == 1){ // check if pressed
                    chip8->pc +=2;
                    }
                    break;
                case 0xA1:
                    if(chip8->keypad[chip8->V[x]] == 0){// check if not pressed
                    chip8->pc +=2;
                    }
                    break;
                default:
                    fprintf(stderr, "Unknown opcode: 0x%X\n", opcode);
                    break;
            break;    
            }
        break;
        /*Timers FX07 FX15 FX18*/
        case 0xF:
            switch(nn){
                case 0x07:
                    chip8->V[x] = chip8->delay_timer;
                    chip8->pc +=2;
                    break;
                case 0X15:
                    chip8->delay_timer = chip8->V[x];
                    chip8->pc +=2;
                    break;
                case 0x18:
                    chip8->sound_timer= chip8->V[x];
                    chip8->pc +=2;
                    break;
                /*add to index*/
                case 0X1E:
                    chip8->I += chip8->V[x];
                    chip8->pc +=2;
                    break;
                /*get key*/ 
                case 0X0A:
                    for(int i = 0; i<16; i++){
                        if(chip8->keypad[i] == 1){
                            chip8->V[x] = i;
                            chip8->pc +=2;
                            break;
                        }   
                    }
                break;
                /*font pointer*/
                case 0X29:
                    chip8->I = (chip8->V[x] *5) + 0X50;
                    chip8->pc += 2;
                    break;
                /*Binary Coded Decimal*/
                case 0X33:
                    chip8->memory[chip8->I] = chip8->V[x]/100;
            break;         
            }
        break;
        
        //Register Instructions
        default:
            fprintf(stderr, "Unknown opcode: 0x%X\n", opcode);
            break;
    }
    return 0;
}


