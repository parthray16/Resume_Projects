import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.PriorityQueue;

public class BranchBound {
    private static final int timelimit = 300000;

    public static void solve(Knapsack ks){    
        //start timer
        long sTime = System.nanoTime(); 
        ArrayList<Item> copy = new ArrayList<Item>(ks.items);
        //Sort items by decending value/weight ratio
        Comparator<Item> c = Comparator.comparing(Item::getVWratio);
        Collections.sort(copy, c.reversed());
        //get a lower bound from greedy algorithm
        int lbound = getLowerBound(ks.capacity, copy);
        //Create root node with the max upperbound
        ArrayList<Integer> path = new ArrayList<>();
        float ubound = ks.capacity * (copy.get(0).vwratio);
        float uboundin = 0;
        Node root = new Node(ubound, 0, 0, 0, path);
        Comparator<Node> c2 = Comparator.comparing(Node::getUbound);
        PriorityQueue<Node> pq = new PriorityQueue<Node>(c2.reversed());
        pq.add(root);
        long timer = System.currentTimeMillis();
        try{
            while(!pq.isEmpty() && 
                  System.currentTimeMillis() - timer < timelimit){
                Node parent = pq.peek();
                //check for solution
                if(parent.level == ks.numitems){
                    break;
                }
                Item it = copy.get(parent.level);
                int weight = it.weight + parent.weight;
                int value = it.value + parent.value;
                float ratio = 0;
                if(parent.level + 1 != ks.numitems){
                    ratio = copy.get(parent.level+1).vwratio;
                }
                //left node, we take item if it fits
                if(weight <= ks.capacity){ 
                    uboundin = value + ((ks.capacity - weight) * ratio);
                    if (uboundin >= lbound){
                        ArrayList<Integer> newpath = new ArrayList<Integer>(parent.path);
                        newpath.add(copy.get(parent.level).number); 
                        Node in = new Node(uboundin, weight, value,
                                            parent.level + 1, newpath);
                        pq.add(in);
                    }
                    else{
                        uboundin = 0;
                    }
                }
                else{
                    //infeasible
                    uboundin = 0;
                }
                //right node, we dont take item
                ubound = parent.value + ((ks.capacity - parent.weight) * ratio);
                if(ubound >= lbound){
                    Node ex = new Node(ubound, parent.weight, parent.value,
                                        parent.level + 1, parent.path);
                    pq.add(ex);
                }
                else{
                    ubound = 0;
                }
                //parent is done processing
                pq.remove(parent);
                //check to see if I reach the max depth
                if(parent.level + 1 == ks.numitems){
                    //clean out pq
                    //remove all nodes with a lower bound than a potential solution
                    float max = Math.max(uboundin, ubound);
                    Iterator<Node> node = pq.iterator();
                    while(node.hasNext()){
                        if(node.next().ubound < max){
                            node.remove(); //prune
                        }
                    }
                }
                
            }
        }
        catch(OutOfMemoryError e){
            pq.clear();
            System.out.println("Branch and Bound Failed to OutOfMemoryError");
            return;  
        }
        //end timer
        long ftime = System.nanoTime();
        long runtime = ftime - sTime;

        Node soln = pq.peek();
        //Print out solution
        if(System.currentTimeMillis() - timer > timelimit){
            System.out.println("Branch and Bound was terminated early");
        }
        System.out.println("Using Branch and Bound the best feasible solution found:  Value " + soln.value + ", Weight " + soln.weight);
        Collections.sort(soln.path);
        for(int i = 0; i < soln.path.size(); i++){
            if(i == soln.path.size() - 1){
                System.out.printf("%d\n", soln.path.get(i));
            }
            else{
			    System.out.printf("%d ", soln.path.get(i));
            }
        }
        System.out.printf("Runtime: %.8f seconds\n",
                            (float) runtime / 1_000_000_000);
        System.out.println();
    }

    public static int getLowerBound(int capacity, ArrayList<Item> copy){
        int solnV = 0;
        int solnW = 0;
        //largest value/weight ratio first
        for (Item item : copy) {
            if((solnW + item.weight) <= capacity){
                //add item if it fits
                solnV += item.value;
                solnW += item.weight;
            }
        }
        return solnV;
    }
}
