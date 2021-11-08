/* Simulates an 8 by 1 multiplexor. Accepts 8 input bits along with 3 control bits and outputs
 * the input bit corresponding to the specified control bits.
 *
 * Author: Yosef Jacobson
 * NetID: yjacobson@email.arizona.edu
 * Instructor: Tyler Conklin
 */

public class Sim3_MUX_8by1 {
    // inputs
    public RussWire[] control, in;
    // output
    public RussWire out;

    public Sim3_MUX_8by1() {
        control = new RussWire[3];
        for (int i = 0; i < 3; i++)
            control[i] = new RussWire();

        in = new RussWire[8];
        for (int i = 0; i < 8; i++)
            in[i] = new RussWire();

        out = new RussWire();
    }

    public void execute() {
        // naming all inputs for easier coding
        boolean c0 = control[0].get(), c1 = control[1].get(), c2 = control[2].get();
        boolean i0 = in[0].get(), i1 = in[1].get(), i2 = in[2].get(), i3 = in[3].get(), i4 = in[4].get(),
                i5 = in[5].get(), i6 = in[6].get(), i7 = in[7].get();
        
        // calculating output bits from input and control bits using a sum of products equation
        boolean in0Out = i0 && !c0 && !c1 && !c2,
                in1Out = i1 && c0 && !c1 && !c2,
                in2Out = i2 && !c0 && c1 && !c2,
                in3Out = i3 && c0 && c1 && !c2,
                in4Out = i4 && !c0 && !c1 && c2,
                in5Out = i5 && c0 && !c1 && c2,
                in6Out = i6 && !c0 && c1 && c2,
                in7Out = i7 && c0 && c1 && c2;
        
        // setting final output
        out.set(in0Out || in1Out || in2Out || in3Out || in4Out || in5Out || in6Out || in7Out);
    }
}
