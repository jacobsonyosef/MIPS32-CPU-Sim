/* Simulates a 1-bit ALU. Accepts 3 bits as an operation code, the two bits to be manipulated,
 * and a carryIn value. Performs AND, OR, ADD, LESS, and XOR on a and b, and feeds the results
 * to an 8 by 1 MUX along with the operation code in order to set result to the desired value.
 * 
 * addResult and carryOut are used internally for operations.
 *
 * Author: Yosef Jacobson
 * NetID: yjacobson@email.arizona.edu
 * Instructor: Tyler Conklin
 */

public class Sim3_ALUElement {
    // inputs
    public RussWire[] aluOp;
    public RussWire bInvert, a, b, carryIn, less;
    // outputs
    public RussWire result, addResult, carryOut;

    // internal components
    public FullAdder adder;
    public Sim3_MUX_8by1 mux;
    public XOR xor;

    public Sim3_ALUElement() {
        aluOp = new RussWire[3];
        for (int i = 0; i < 3; i++)
            aluOp[i] = new RussWire();

        bInvert = new RussWire();
        a = new RussWire();
        b = new RussWire();
        carryIn = new RussWire();
        less = new RussWire();

        result = new RussWire();
        addResult = new RussWire();
        carryOut = new RussWire();

        adder = new FullAdder();
        mux = new Sim3_MUX_8by1();
        for (int i = 5; i < 8; i++)
            mux.in[i].set(false); // since the last 3 elements of in[] aren't used by the mux, set them to default
                                  // values to prevent errors

        xor = new XOR();
    }

    public void execute_pass1() {
        // using XOR to invert b according to bInvert
        xor.a.set(this.b.get());
        xor.b.set(bInvert.get());
        xor.execute();
        boolean finalB = xor.out.get();

        // setting values for and executing adder
        adder.a.set(this.a.get());
        adder.b.set(finalB);
        adder.carryIn.set(this.carryIn.get());
        adder.execute();

        // setting valid outputs
        addResult.set(adder.sum.get());
        carryOut.set(adder.carryOut.get());
        mux.in[0].set(a.get() & finalB);    // result for AND
        mux.in[1].set(a.get() | finalB);    // result for OR
        mux.in[2].set(addResult.get());     // result for ADD
        mux.in[4].set(a.get() ^ finalB);    // result for XOR
    }

    public void execute_pass2() {
        mux.in[3].set(less.get());  // result for LESS

        // selecting correct result using control input and the mux
        mux.control = aluOp;
        mux.execute();

        result.set(mux.out.get()); // setting the desired result
    }
}
