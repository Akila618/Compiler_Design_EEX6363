#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "isa2.h"
#include "codegen.h"
#include "stack.h"

void generate_isa2_target() {
    FILE* out = fopen("files/object_code.txt", "w");
    if (!out) return;
    
    Quadruple* q = get_quad_list();
    int count = get_quad_count();
    
    fprintf(out, "============================================================\n");
    fprintf(out, "       ISA: Accumulator Assembly\n");
    fprintf(out, "============================================================\n");
    fprintf(out, "  stack pointer, base pointer have reserved addresses\n");
    fprintf(out, "  seperate region for stack data memory\n");
    fprintf(out, "  seperate region for global variables\n");
    fprintf(out, "============================================================\n");
    fprintf(out, "\n");
    fprintf(out, "main_entry:\n");
    fprintf(out, "        loadacc #%d\n", STACK_BASE); 
    fprintf(out, "        storeacc %d\n", VREG_SP);
    fprintf(out, "        storeacc %d\n", VREG_BP); 
    fprintf(out, "\n");

    for (int i=0; i<count; i++) {
        char* op = q[i].op ? q[i].op : "";
        
        if (strcmp(op, "label")==0) {
            fprintf(out, "%s:\n", q[i].res);
            continue;
        }

        if (strcmp(op, "goto")==0) {
            fprintf(out, "        jump %s\n", q[i].res);
            continue;
        }
        
        if (strcmp(op, "pushBP")==0) {
            // push base pointer onto stack
            fprintf(out, "        loadacc %d\n", VREG_BP);
            fprintf(out, "        storeacc &%d\n", VREG_SP);
            fprintf(out, "        loadacc %d\n", VREG_SP);
            fprintf(out, "        add #1\n");
            fprintf(out, "        storeacc %d\n", VREG_SP);
            continue;
        }
        
        if (strcmp(op, "setBP")==0) {
            // assign SP to BP
            fprintf(out, "        loadacc %d\n", VREG_SP);
            fprintf(out, "        storeacc %d\n", VREG_BP);
            continue;
        }
        
        if (strcmp(op, "allocFrame")==0) {
            // SP := SP + frameSize (for local variables)
            fprintf(out, "        loadacc %d\n", VREG_SP);
            fprintf(out, "        add #%s\n", q[i].arg2);
            fprintf(out, "        storeacc %d\n", VREG_SP);
            continue;
        }
        
        if (strcmp(op, "restoreSP")==0) {
            // assign BP to SP
            fprintf(out, "        loadacc %d\n", VREG_BP);
            fprintf(out, "        storeacc %d\n", VREG_SP);
            continue;
        }
        
        if (strcmp(op, "popBP")==0) {
            // SP := SP - 1; BP := MEM[SP]
            fprintf(out, "        loadacc %d\n", VREG_SP);
            fprintf(out, "        sub #1\n");
            fprintf(out, "        storeacc %d\n", VREG_SP);
            fprintf(out, "        loadacc &%d\n", VREG_SP);
            fprintf(out, "        storeacc %d\n", VREG_BP);
            continue;
        }
        
        // =============================== access stack ================================
        if (strcmp(op, "frameAddr")==0) {
            // frame address: res = BP + offset
            fprintf(out, "        loadacc %d\n", VREG_BP);
            int offset = atoi(q[i].arg2);
            if (offset >= 0) {
                fprintf(out, "        add #%d\n", offset);
            } else {
                fprintf(out, "        sub #%d\n", -offset);
            }
            int resAddr = get_var_address(q[i].res);
            fprintf(out, "        storeacc %d\n", resAddr);
            continue;
        }
        
        if (strcmp(op, "loadStack")==0) {
            // load from stack
            int addrTemp = get_var_address(q[i].arg1);
            int resAddr = get_var_address(q[i].res);
            fprintf(out, "        loadacc %d\n", addrTemp);
            fprintf(out, "        storeacc %d\n", VREG_PTR);
            fprintf(out, "        loadacc &%d\n", VREG_PTR);
            fprintf(out, "        storeacc %d\n", resAddr);
            continue;
        }
        
        if (strcmp(op, "storeStack")==0) {
            // store to stack
            int valAddr = get_var_address(q[i].arg1);
            int addrTemp = get_var_address(q[i].res);
            fprintf(out, "        loadacc %d\n", addrTemp);
            fprintf(out, "        storeacc %d\n", VREG_PTR);
            if (valAddr == -1) {
                fprintf(out, "        loadacc #%s\n", q[i].arg1);
            } else {
                fprintf(out, "        loadacc %d\n", valAddr);
            }
            fprintf(out, "        storeacc &%d\n", VREG_PTR);
            continue;
        }
        
        if (strcmp(op, "call")==0) {
            fprintf(out, "        call %s\n", q[i].arg1);
            int r = get_var_address(q[i].res);
            fprintf(out, "        storeacc %d\n", r);
            continue;
        }
        if (strcmp(op, "return")==0) {
            if (q[i].arg1) {
                int v = get_var_address(q[i].arg1);
                if (v==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
                else fprintf(out, "        loadacc %d\n", v);
            }
            fprintf(out, "        ret\n");
            continue;
        }

        // --- Assignment operations ---
        if (strcmp(op, "assign")==0) {
            int s = get_var_address(q[i].arg1);
            int d = get_var_address(q[i].res);
            if (s==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
            else fprintf(out, "        loadacc %d\n", s);
            fprintf(out, "        storeacc %d\n", d);
            continue;
        }

        // --- Array operations---
        if (strcmp(op, "store")==0) { 
            int b = get_var_address(q[i].res);
            int o = get_var_address(q[i].arg2);
            int v = get_var_address(q[i].arg1);
            fprintf(out, "        loadacc #%d\n", b);
            fprintf(out, "        add %d\n", o);
            int p = get_var_address("PTR");
            fprintf(out, "        storeacc %d\n", p);
            if (v==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
            else fprintf(out, "        loadacc %d\n", v);
            fprintf(out, "        storeacc &%d\n", p);
            continue;
        }
        if (strcmp(op, "load")==0) { 
            int b = get_var_address(q[i].arg1);
            int o = get_var_address(q[i].arg2);
            int d = get_var_address(q[i].res);
            fprintf(out, "        loadacc #%d\n", b);
            fprintf(out, "        add %d\n", o);
            int p = get_var_address("PTR");
            fprintf(out, "        storeacc %d\n", p);
            fprintf(out, "        loadacc &%d\n", p);
            fprintf(out, "        storeacc %d\n", d);
            continue;
        }

        // --- Arithmetic operations---
        if (strcmp(op, "+")==0 || strcmp(op, "-")==0 || 
            strcmp(op, "*")==0 || strcmp(op, "/")==0) {
            
            int a1 = get_var_address(q[i].arg1);
            int a2 = get_var_address(q[i].arg2);
            int r = get_var_address(q[i].res);
            
            if (a1==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
            else fprintf(out, "        loadacc %d\n", a1);
            
            char* cmd = "add";
            if (strcmp(op, "-")==0) cmd = "sub";
            if (strcmp(op, "*")==0) cmd = "mul";
            if (strcmp(op, "/")==0) cmd = "div";
            
            if (a2==-1) fprintf(out, "        %s #%s\n", cmd, q[i].arg2);
            else fprintf(out, "        %s %d\n", cmd, a2);
            
            fprintf(out, "        storeacc %d\n", r);
            continue;
        }

        // --- Logical operations---
        if (strcmp(op, "and")==0 || strcmp(op, "or")==0 || strcmp(op, "xor")==0) {
            int a1 = get_var_address(q[i].arg1);
            int a2 = get_var_address(q[i].arg2);
            int r = get_var_address(q[i].res);

            if (a1==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
            else fprintf(out, "        loadacc %d\n", a1);

            char* cmd = "and";
            if (strcmp(op, "or")==0) cmd = "or";
            if (strcmp(op, "xor")==0) cmd = "xor";

            if (a2==-1) fprintf(out, "        %s #%s\n", cmd, q[i].arg2);
            else fprintf(out, "        %s %d\n", cmd, a2);

            fprintf(out, "        storeacc %d\n", r);
            continue;
        }

        if (strcmp(op, "not")==0) {
            int a1 = get_var_address(q[i].arg1);
            int r = get_var_address(q[i].res);
            if (a1==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
            else fprintf(out, "        loadacc %d\n", a1);
            fprintf(out, "        not\n");
            fprintf(out, "        storeacc %d\n", r);
            continue;
        }

        // --- Condition operations---
        if (strcmp(op, "==")==0 || strcmp(op, "<")==0 || strcmp(op, ">")==0 || 
            strcmp(op, ">=")==0 || strcmp(op, "<=")==0) {
            int a1 = get_var_address(q[i].arg1);
            int a2 = get_var_address(q[i].arg2);
            
            if (a1==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
            else fprintf(out, "        loadacc %d\n", a1);
            
            if (a2==-1) fprintf(out, "        sub #%s\n", q[i].arg2);
            else fprintf(out, "        sub %d\n", a2);
            
            if (strcmp(op, "==")==0) fprintf(out, "        jz %s\n", q[i].res);
            if (strcmp(op, "<")==0) fprintf(out, "        js %s\n", q[i].res);
             
            //jump combinations:
            if (strcmp(op, ">")==0) {
                fprintf(out, "        jz _skip_%d\n", i); 
                fprintf(out, "        js _skip_%d\n", i); 
                fprintf(out, "        jump %s\n", q[i].res); 
                fprintf(out, "_skip_%d: nop\n", i);
            }
            continue;
        }
        
        if (strcmp(op, "write")==0) {
            int v = get_var_address(q[i].arg1);
            if (v==-1) fprintf(out, "        loadacc #%s\n", q[i].arg1);
            else fprintf(out, "        loadacc %d\n", v);
            fprintf(out, "        storeacc 5000\n");
            continue;
        }
    }
    fprintf(out, "        hlt\n");
    fclose(out);

    printf("======================================================================================================\n");
    printf("[ISA: OK]: Target code generation for acc ISA completed.\n");
    printf("======================================================================================================\n");
}