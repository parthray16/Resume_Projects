import java.util.ArrayList;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;

public class Knapsack {
    public int numitems;
    public int capacity;
    public ArrayList<Item> items;

    public Knapsack(String filename){
        try{
            File problem = new File(filename);
            Scanner sc = new Scanner(problem);
            numitems = sc.nextInt();
            int i = numitems;
            items = new ArrayList<Item>();
            while(i != 0){
                if(sc.hasNextInt()){
                    Item it = new Item(sc.nextInt(), sc.nextInt(), sc.nextInt());
                    items.add(it);
                    i--;
                }
                else{
                    System.out.println("Bad Problem File");
                    System.exit(1);
                }
            }
            capacity = sc.nextInt();
            sc.close();
        }
        catch(FileNotFoundException e){
            System.out.println("File: " + filename + " does not exist.");
            System.exit(1);
        }
    }
}