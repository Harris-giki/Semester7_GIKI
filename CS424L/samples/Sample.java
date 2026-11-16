// A small Java program used to test the identifier finders.
public class Sample {
    private int total = 0;

    public int add(int value) {
        total = total + value; // running sum
        return total;
    }

    public static void main(String[] args) {
        Sample s = new Sample();
        s.add(10);
        s.add(-20);
        System.out.println(s.add(3));
    }
}
