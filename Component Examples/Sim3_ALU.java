/* Simulates an X-bit ALU, where X is given as a constructor argument. Inputs are two
 * X-bit integers represented as arrays of bits (a and b), a 3-bit operation code (aluOp),
 * and bNegate to indicate b should be negated. Outputs the desired result as an X-sized
 * array of bits (result), with the desired operation result being indicated by aluOp.
 * 
 *
 * Author: Yosef Jacobson
 * NetID: yjacobson@email.arizona.edu
 * Instructor: Tyler Conklin
 */

public class Sim3_ALU {
    // inputs
    public RussWire[] aluOp, a, b;
    public RussWire bNegate;
    public int numBits;
    // output
    public RussWire[] result;

    // internal components
    public Sim3_ALUElement[] elements;

    public Sim3_ALU(int X) {
        numBits = X;

        aluOp = new RussWire[3];
        for (int i = 0; i < 3; i++)
            aluOp[i] = new RussWire();
        
        a = new RussWire[numBits];
        b = new RussWire[numBits];
        result = new RussWire[numBits];

        elements = new Sim3_ALUElement[numBits];

        for (int i = 0; i < numBits; i++) {
            a[i] = new RussWire();
            b[i] = new RussWire();
            result[i] = new RussWire();

            elements[i] = new Sim3_ALUElement();
        }

        bNegate = new RussWire();
    }

    public void execute() {
        // calculating first pass of the first column
        elements[0].aluOp = this.aluOp;
        elements[0].bInvert.set(bNegate.get());
        elements[0].a.set(a[0].get());
        elements[0].b.set(b[0].get());
        elements[0].carryIn.set(bNegate.get());
        elements[0].execute_pass1();

        // and the rest
        int i = 1;
        for (; i < numBits; i++) {
            elements[i].aluOp = this.aluOp;
            elements[i].bInvert = bNegate;
            elements[i].a.set(a[i].get());
            elements[i].b.set(b[i].get());
            elements[i].carryIn.set(elements[i - 1].carryOut.get());
            elements[i].less.set(false); // less = 0 for every ALU but the first, so it can be safely set here
            elements[i].execute_pass1();
        }
        
        // setting less for first ALU to be addResult of final ALU
        elements[0].less.set(elements[i - 1].addResult.get());
        
        // second pass
        for (i = 0; i < numBits; i++) {
            elements[i].execute_pass2();
            
            result[i].set(elements[i].result.get()); // setting final result
        }
    }
}
