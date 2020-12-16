import java.util.ArrayList;
import java.util.Collections;

public class DynamicProgramming {
    public static void solve(Knapsack ks){
        //start timer
        long sTime = System.nanoTime(); 
        //Create table for problem size
        int table[][];
        try{
            table = new int[ks.numitems + 1][ks.capacity + 1];
            //Fill in table using our recurrence relation
            for(int i = 0; i <= ks.numitems; i++){
                for(int j = 0; j <= ks.capacity; j++){
                    if(i == 0){
                        //base case
                        table[i][j] = 0;
                    }
                    else if (j == 0){
                        //base case
                        table[i][j] = 0;
                    }
                    else{
                        //Apply Recurrence Relation
                        Item it = ks.items.get(i - 1);
                        if((j - it.weight) >= 0){
                            //if item fits
                            table[i][j] = Math.max(table[i - 1][j],
                                        table[i - 1][j - it.weight] + it.value);
                        }
                        else{
                            table[i][j] = table[i - 1][j];
                        }
                    }
                }
            }
        }
        catch(OutOfMemoryError e){
            System.out.println("Dynamic Programming Failed to OutOfMemoryError");
            System.out.println();
            return;
        }
        //Trace Back
        ArrayList<Integer> solnSet = new ArrayList<>();
        int solnV = 0;
        int solnW = 0;
        int j = ks.capacity;
        for(int i = ks.numitems; i > 0; i--){
            if(table[i][j] == table[i-1][j]){
                //item not taken
                continue;
            }
            else{
                //item taken
                Item it = ks.items.get(i - 1);
                solnSet.add(it.number);
                solnV += it.value;
                solnW += it.weight;
                j -= it.weight;
                if(j <= 0){
                    //end of table
                    break;
                }
            }
        }
        //end timer
        long ftime = System.nanoTime();
        long runtime = ftime - sTime;
        //Print out solution
        System.out.println("Dynamic Programming solution:  Value " + solnV + ", Weight " + solnW);
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
