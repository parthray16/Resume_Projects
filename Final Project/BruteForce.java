import java.util.ArrayList;

public class BruteForce {

    public static void solve(Knapsack ks){
        //start timer
        long sTime = System.nanoTime(); 
        int solnV = bfHelper(ks.capacity, ks.numitems, ks.items);
        //end timer
        long ftime = System.nanoTime();
        long runtime = ftime - sTime;
        //Print out solution
        System.out.println("Using Brute Force the best feasible solution found:  Value " + solnV + ", Weight " + solnW);
        System.out.printf("Runtime: %.8f seconds\n",
            (float) runtime / 1_000_000_000);
        System.out.println();

    }
    
    public static int bfHelper(int capacity, int numitems,
                                ArrayList<Item> items){
        if(numitems == 0 || capacity <= 0){
            return 0;
        }
        if(items.get(numitems - 1).weight > capacity){
            return bfHelper(capacity, numitems - 1, items);
        }
        else{
            int in = bfHelper(capacity - 
                    items.get(numitems - 1).weight, numitems - 1, items);
            in += items.get(numitems - 1).value;
            int out = bfHelper(capacity, numitems - 1, items);
            if(in >= out){
                return in;
            }
            else{
                return out;
            }
        }
    }
}
