public class TestFile {

    public static void main(String[] args){
        System.out.println("\n-------mystery20--------");
        Knapsack ks0 = new Knapsack("mystery20.txt");
        GreedySearch.solve(ks0);
        DynamicProgramming.solve(ks0);
        BranchBound.solve(ks0);

        System.out.println("\n-------hard40--------");
        Knapsack ks1 = new Knapsack("hard40.txt");
        GreedySearch.solve(ks1);
        DynamicProgramming.solve(ks1);
        BranchBound.solve(ks1);
    /*    
        System.out.println("\n-------easy20--------");
        Knapsack ks2 = new Knapsack("easy20.txt");
        BruteForce.solve(ks2);
        GreedySearch.solve(ks2);
        DynamicProgramming.solve(ks2);
        BranchBound.solve(ks2);

        System.out.println("\n-------easy50--------");
        Knapsack ks3 = new Knapsack("easy50.txt");
        GreedySearch.solve(ks3);
        DynamicProgramming.solve(ks3);
        BranchBound.solve(ks3);

        System.out.println("\n-------hard50--------");
        Knapsack ks4 = new Knapsack("hard50.txt");
        GreedySearch.solve(ks4);
        DynamicProgramming.solve(ks4);
        BranchBound.solve(ks4);

        System.out.println("\n-------easy200--------");
        Knapsack ks5 = new Knapsack("easy200.txt");
        GreedySearch.solve(ks5);
        DynamicProgramming.solve(ks5);
        BranchBound.solve(ks5);

        System.out.println("\n-------hard200--------");
        Knapsack ks6 = new Knapsack("hard200.txt");
        GreedySearch.solve(ks6);
        DynamicProgramming.solve(ks6);
        BranchBound.solve(ks6);
    */
    }
}
