import java.util.ArrayList;

public class Node {
    public float ubound;
    public int weight;
    public int value;
    public int level;
    public ArrayList<Integer> path;

    public Node(float ubound, int weight, int value, 
                int level, ArrayList<Integer> path){
        this.ubound = ubound;
        this.weight = weight;
        this.value = value;
        this.level = level;
        this.path = path;
    }
    public float getUbound(){return ubound;}
}
