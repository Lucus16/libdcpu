#include "main.h"
#include "disassembly.h"

int getDisassembly(char* dest, DCPU* dcpu, word pos) {
    const char* const aValues[64] = {
        "A", "B", "C", "X", "Y", "Z", "I", "J",
        "[A]", "[B]", "[C]", "[X]", "[Y]", "[Z]", "[I]", "[J]",
        "[A+0x%.4x]", "[B+0x%.4x]", "[C+0x%.4x]", "[X+0x%.4x]", "[Y+0x%.4x]", "[Z+0x%.4x]", "[I+0x%.4x]", "[J+0x%.4x]",
        "pop", "[SP]", "[SP+0x%.4x]", "SP", "PC", "EX", "[0x%.4x]", "0x%.4x",
        "-1", "0", "1", "2", "3", "4", "5", "6",
        "7", "8", "9", "10", "11", "12", "13", "14",
        "15", "16", "17", "18", "19", "20", "21", "22",
        "23", "24", "25", "26", "27", "28", "29", "30"
    };
    const char* const bValues[32] = {
        "A", "B", "C", "X", "Y", "Z", "I", "J",
        "[A]", "[B]", "[C]", "[X]", "[Y]", "[Z]", "[I]", "[J]",
        "[A+0x%.4x]", "[B+0x%.4x]", "[C+0x%.4x]", "[X+0x%.4x]", "[Y+0x%.4x]", "[Z+0x%.4x]", "[I+0x%.4x]", "[J+0x%.4x]",
        "push", "[SP]", "[SP+0x%.4x]", "SP", "PC", "EX", "[0x%.4x]", "0x%.4x"
    };
    const char opcodes[32][4] = {
        "adv", "set", "add", "sub", "mul", "mli", "div", "dvi",
        "mod", "mdi", "and", "bor", "xor", "shr", "asr", "shl",
        "ifb", "ifc", "ife", "ifn", "ifg", "ifa", "ifl", "ifu",
        "inv", "inv", "adx", "sbx", "inv", "inv", "sti", "std"
    };
    const char advops[32][4] = {
        "inv", "jsr", "inv", "inv", "inv", "inv", "inv", "inv",
        "int", "iag", "ias", "rfi", "iaq", "inv", "inv", "inv",
        "hwn", "hwq", "hwi", "inv", "inv", "inv", "inv", "inv",
        "inv", "inv", "inv", "inv", "inv", "inv", "inv", "inv"
    };
    word startpos = pos;
    word instruction = dcpu->mem[pos++];
    int opcode = instruction & 0x1f;
    int arga = instruction >> 10;
    int argb = (instruction >> 5) & 0x1f;
    int len = 1;
    char tmpstr[80];
    char astr[12];
    char bstr[12];

    if ((strncmp(opcodes[opcode], "inv", 3) == 0) || (opcode == 0 && strncmp(advops[argb], "inv", 3) == 0)) {
        while (dcpu->mem[pos++] == instruction && len < 65536) { len++; }
        if (len > 15) {
            snprintf(tmpstr, 80, ".fill %i 0x%.4x", len, instruction);
            snprintf(dest, 80, "%-40s;0x%.4x-0x%.4x: 0x%.4x\n", tmpstr, startpos, (startpos + len) & 0xffff, instruction);
            return len;
        } else {
            snprintf(tmpstr, 80, ".dat 0x%.4x", instruction);
            snprintf(dest, 80, "%-40s;0x%.4x: 0x%.4x\n", tmpstr, startpos, instruction);
            return 1;
        }
    } else if (opcode != 0) {
        if (strchr(aValues[arga], '%') != NULL) {
            snprintf(astr, 12, aValues[arga], dcpu->mem[pos++]);
            len++;
        } else {
            strncpy(astr, aValues[arga], 12);
        }
        if (strchr(bValues[argb], '%') != NULL) {
            snprintf(bstr, 12, bValues[argb], dcpu->mem[pos++]);
            len++;
        } else {
            strncpy(bstr, bValues[argb], 12);
        }
        snprintf(tmpstr, 80, "%s %s, %s", opcodes[opcode], bstr, astr);
        if (len == 1) {
            snprintf(dest, 80, "%-40s;0x%.4x: 0x%.4x\n", tmpstr, startpos, dcpu->mem[startpos]);
        } else if (len == 2) {
            snprintf(dest, 80, "%-40s;0x%.4x: 0x%.4x, 0x%.4x\n", tmpstr, startpos, dcpu->mem[startpos], dcpu->mem[(startpos + 1) & 0xffff]);
        } else if (len == 3) {
            snprintf(dest, 80, "%-40s;0x%.4x: 0x%.4x, 0x%.4x, 0x%.4x\n",
                tmpstr, startpos, dcpu->mem[startpos], dcpu->mem[(startpos + 1) & 0xffff], dcpu->mem[(startpos + 2) & 0xffff]);
        }
        return len;
    } else {
        if (strchr(aValues[arga], '%') != NULL) {
            snprintf(astr, 12, aValues[arga], dcpu->mem[pos++]);
            len++;
        } else {
            strncpy(astr, aValues[arga], 12);
        }
        snprintf(tmpstr, 80, "%s %s", advops[argb], astr);
        if (len == 1) {
            snprintf(dest, 80, "%-40s;0x%.4x: 0x%.4x\n", tmpstr, startpos, dcpu->mem[startpos]);
        } else if (len == 2) {
            snprintf(dest, 80, "%-40s;0x%.4x: 0x%.4x, 0x%.4x\n", tmpstr, startpos, dcpu->mem[startpos], dcpu->mem[(startpos + 1) & 0xffff]);
        }
        return len;
    }
}
