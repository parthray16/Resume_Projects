import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

public class GreedySearch {
    public static void solve(Knapsack ks){
        //start timer
        long sTime = System.nanoTime(); 
        //Take greatest value/weight ratio that fits.
        ArrayList<Item> copy = new ArrayList<Item>(ks.items);
        //Sort items by decending value/weight ratio
        Comparator<Item> c = Comparator.comparing(Item::getVWratio);
        Collections.sort(copy, c.reversed());
        ArrayList<Integer> solnSet = new ArrayList<>();
        int solnV = 0;
        int solnW = 0;
        for (Item item : copy) {
            if((solnW + item.weight) <= ks.capacity){
                //add item if it fits
                solnV += item.value;
                solnW += item.weight;
                solnSet.add(item.number);
            }
        }
        //end timer
        long ftime = System.nanoTime();
        long runtime = ftime - sTime;
        //Print out solution
        System.out.println("Greedy solution (not necessarily optimal):  Value " + solnV + ", Weight " + solnW);
        Collections.sort(solnSet);
        for(int i = 0; i < solnSet.size(); i++){
            if(i == solnSet.size() - 1){
                System.out.printf("%d\n", solnSet.get(i));
            }
            else{
			    System.out.printf("%d ", solnSet.get(i));
            }
        }
        System.out.printf("Runtime: %.8f seconds\n",
                        (float) runtime / 1_000_000_000);
        System.out.println();
    }
}
