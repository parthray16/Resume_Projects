public class Item{
    public int number;
    public int value;
    public int weight;
    public float vwratio;

    public Item(int number, int value, int weight){
        this.number = number;
        this.value = value;
        this.weight = weight;
        this.vwratio = (float)value / (float)weight;
    }
    public String toString(){
        return number + "    " + value + "    " + weight + "    " + vwratio;
    }
    public float getVWratio(){return vwratio;}
}
